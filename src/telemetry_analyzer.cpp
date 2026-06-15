#include "telemetry_analyzer.hpp"
#include <algorithm>
#include <execution>
#include <fstream>
#include <future>
#include <iomanip>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <thread>

namespace Telemetry {

namespace {
constexpr double kKmHToMs = 1000.0 / 3600.0;
} // namespace

TelemetryAnalyzer::DerivedSummary TelemetryAnalyzer::combineSummaries(
    const DerivedSummary &left,
    const DerivedSummary &right) {
  DerivedSummary combined = left;
  combined.sumSpeed += right.sumSpeed;
  combined.sumAcceleration += right.sumAcceleration;
  combined.maxLateralG = std::max(left.maxLateralG, right.maxLateralG);
  combined.overspeed += right.overspeed;
  combined.highRpm += right.highRpm;
  combined.hardBraking += right.hardBraking;
  combined.curveEvents += right.curveEvents;
  combined.sampleCount += right.sampleCount;
  combined.accelerationSamples += right.accelerationSamples;
  if (!combined.hasMaxRpm || right.maxRpm.rpm > combined.maxRpm.rpm) {
    combined.maxRpm = right.maxRpm;
    combined.hasMaxRpm = right.hasMaxRpm;
  }
  return combined;
}

TelemetryAnalyzer::DerivedSummary TelemetryAnalyzer::processChunk(
    const std::vector<TelemetryRecord> &data,
    size_t first,
    size_t last,
    const TelemetryRecord *previous,
    double speedLimit,
    int32_t rpmLimit,
    double brakingThreshold,
    double curveGThreshold) {
  DerivedSummary summary;
  if (first >= last)
    return summary;

  summary.maxRpm = data[first];
  summary.hasMaxRpm = true;
  const TelemetryRecord *prev = previous;

  for (size_t index = first; index < last; ++index) {
    const TelemetryRecord &current = data[index];
    summary.sumSpeed += current.speed;
    summary.sampleCount += 1;

    if (current.rpm > summary.maxRpm.rpm)
      summary.maxRpm = current;

    // Branchless: convert boolean to int directly
    summary.overspeed += static_cast<int>(current.speed > speedLimit);
    summary.highRpm += static_cast<int>(current.rpm > rpmLimit);

    if (prev) {
      double dt = static_cast<double>(current.timestamp - prev->timestamp) / 1000.0;
      if (dt > 0.0) {
        double dv = (current.speed - prev->speed) * kKmHToMs;
        double acceleration = dv / dt;
        summary.sumAcceleration += acceleration;
        summary.accelerationSamples += 1;

        // Branchless: convert boolean to int directly
        summary.hardBraking += static_cast<int>(acceleration < brakingThreshold);

        double rpmDelta = static_cast<double>(current.rpm - prev->rpm);
        double lateralG = std::min(4.0, std::abs(dv) * std::abs(rpmDelta) / 2500.0);
        summary.maxLateralG = std::max(summary.maxLateralG, lateralG);

        // Branchless: convert boolean to int directly
        summary.curveEvents += static_cast<int>(lateralG >= curveGThreshold);
      }
    }

    prev = &current;
  }

  return summary;
}

TelemetryAnalyzer::TelemetryAnalyzer(const std::vector<TelemetryRecord> &data)
    : m_data(data) {}

double TelemetryAnalyzer::getAverageSpeed() const {
  if (m_data.empty())
    return 0.0;

  double sum = std::transform_reduce(
      std::execution::par_unseq,
      m_data.begin(),
      m_data.end(),
      0.0,
      std::plus<>(),
      [](const TelemetryRecord &record) { return record.speed; });

  return sum / static_cast<double>(m_data.size());
}

TelemetryRecord TelemetryAnalyzer::getMaxRPM() const {
  if (m_data.empty())
    return TelemetryRecord{};

  return *std::max_element(
      std::execution::par_unseq,
      m_data.begin(),
      m_data.end(),
      [](const TelemetryRecord &lhs, const TelemetryRecord &rhs) {
        return lhs.rpm < rhs.rpm;
      });
}

int TelemetryAnalyzer::countOverspeedEvents(double speedLimit) const {
  return std::transform_reduce(
      std::execution::par_unseq,
      m_data.begin(),
      m_data.end(),
      0,
      std::plus<>(),
      [speedLimit](const TelemetryRecord &record) {
        return static_cast<int>(record.speed > speedLimit);
      });
}

int TelemetryAnalyzer::countHighRpmEvents(int32_t rpmLimit) const {
  return std::transform_reduce(
      std::execution::par_unseq,
      m_data.begin(),
      m_data.end(),
      0,
      std::plus<>(),
      [rpmLimit](const TelemetryRecord &record) {
        return static_cast<int>(record.rpm > rpmLimit);
      });
}

// CORRIGIDO: Escopo completo do tipo de retorno adicionado
TelemetryAnalyzer::DerivedSummary TelemetryAnalyzer::computeDerivedSummary(
    double speedLimit,
    int32_t rpmLimit,
    double brakingThreshold,
    double curveGThreshold) const {
  DerivedSummary globalSummary;
  if (m_data.empty())
    return globalSummary;

  const size_t dataSize = m_data.size();
  const size_t threadCount = std::max<size_t>(1, std::thread::hardware_concurrency());
  const size_t chunkCount = std::min(threadCount, dataSize);
  const size_t chunkSize = (dataSize + chunkCount - 1) / chunkCount;

  std::vector<std::future<DerivedSummary>> futures;
  futures.reserve(chunkCount);

  for (size_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex) {
    size_t first = chunkIndex * chunkSize;
    size_t last = std::min(first + chunkSize, dataSize);
    const TelemetryRecord *previous = (first > 0 ? &m_data[first - 1] : nullptr);

    futures.emplace_back(std::async(std::launch::async,
      [this, first, last, previous, speedLimit, rpmLimit, brakingThreshold,
        curveGThreshold]() {
        return processChunk(m_data, first, last, previous, speedLimit, rpmLimit,
                            brakingThreshold, curveGThreshold);
      }));
  }

  for (auto &future : futures) {
    DerivedSummary chunkSummary = future.get();
    globalSummary = combineSummaries(globalSummary, chunkSummary);
  }

  return globalSummary;
}

int TelemetryAnalyzer::countHardBrakingEvents(double threshold) const {
  return computeDerivedSummary(0.0, 0, threshold, std::numeric_limits<double>::infinity()).hardBraking;
}

int TelemetryAnalyzer::countCurveEvents(double lateralGThreshold) const {
  return computeDerivedSummary(0.0, 0, std::numeric_limits<double>::lowest(), lateralGThreshold).curveEvents;
}

double TelemetryAnalyzer::getAverageAcceleration() const {
  DerivedSummary summary = computeDerivedSummary(0.0, 0, std::numeric_limits<double>::lowest(), 0.0);
  if (summary.accelerationSamples == 0)
    return 0.0;
  return summary.sumAcceleration / static_cast<double>(summary.accelerationSamples);
}

double TelemetryAnalyzer::getMaxLateralG() const {
  DerivedSummary summary = computeDerivedSummary(0.0, 0, std::numeric_limits<double>::lowest(), 0.0);
  return summary.maxLateralG;
}

void TelemetryAnalyzer::exportToJSON(const std::string &filename,
                                     double speedLimit,
                                     int32_t rpmLimit) const {
  // Forward to the extended overload with no custom channel info
  exportToJSON(filename, speedLimit, rpmLimit, std::string(), 0.0, 0.0);
}

void TelemetryAnalyzer::exportToJSON(const std::string &filename,
                                     double speedLimit,
                                     int32_t rpmLimit,
                                     const std::string &customFormula,
                                     double customMean,
                                     double customMax) const {
  DerivedSummary summary = computeDerivedSummary(speedLimit, rpmLimit, -4.0, 0.5);
  std::ofstream out(filename);
  if (!out.is_open())
    throw std::runtime_error("Nao foi possivel criar o arquivo JSON: " + filename);

  double averageSpeed = summary.sampleCount ? summary.sumSpeed / summary.sampleCount : 0.0;
  double averageAcceleration = summary.accelerationSamples ? summary.sumAcceleration / summary.accelerationSamples : 0.0;

  out << std::fixed << std::setprecision(2);
  out << "{\n";
  out << "  \"records\": " << m_data.size() << ",\n";
  out << "  \"averageSpeed\": " << averageSpeed << ",\n";
  out << "  \"maxRPM\": " << summary.maxRpm.rpm << ",\n";
  out << "  \"averageAcceleration\": " << averageAcceleration << ",\n";
  out << "  \"maxLateralG\": " << summary.maxLateralG << ",\n";
  out << "  \"overspeedEvents\": " << summary.overspeed << ",\n";
  out << "  \"highRpmEvents\": " << summary.highRpm << ",\n";
  out << "  \"hardBrakingEvents\": " << summary.hardBraking << ",\n";
  out << "  \"curveEvents\": " << summary.curveEvents;

  if (!customFormula.empty()) {
    out << ",\n";
    out << "  \"customChannel\": {\n";
    out << "    \"formula\": \"" << customFormula << "\",\n";
    out << "    \"mean\": " << customMean << ",\n";
    out << "    \"max\": " << customMax << "\n";
    out << "  }\n";
  } else {
    out << "\n";
  }

  out << "}\n";
}

void TelemetryAnalyzer::exportToCSV(const std::string &filename,
                                    double speedLimit,
                                    int32_t rpmLimit) const {
  DerivedSummary summary = computeDerivedSummary(speedLimit, rpmLimit, -4.0, 0.5);
  std::ofstream out(filename);
  if (!out.is_open())
    throw std::runtime_error("Nao foi possivel criar o arquivo CSV: " + filename);

  double averageSpeed = summary.sampleCount ? summary.sumSpeed / summary.sampleCount : 0.0;
  double averageAcceleration = summary.accelerationSamples ? summary.sumAcceleration / summary.accelerationSamples : 0.0;

  out << "metric,value\n";
  out << "records," << m_data.size() << "\n";
  out << "averageSpeed," << averageSpeed << "\n";
  out << "maxRPM," << summary.maxRpm.rpm << "\n";
  out << "averageAcceleration," << averageAcceleration << "\n";
  out << "maxLateralG," << summary.maxLateralG << "\n";
  out << "overspeedEvents," << summary.overspeed << "\n";
  out << "highRpmEvents," << summary.highRpm << "\n";
  out << "hardBrakingEvents," << summary.hardBraking << "\n";
  out << "curveEvents," << summary.curveEvents << "\n";
}

void TelemetryAnalyzer::generateReport(const std::string &filename,
                                       double speedLimit,
                                       int32_t rpmLimit) const {
  std::ofstream out(filename);
  if (!out.is_open())
    throw std::runtime_error("Nao foi possivel criar o arquivo de relatorio: " +
                             filename);

  DerivedSummary summary =
      computeDerivedSummary(speedLimit, rpmLimit, -4.0, 0.5);

  out << "=== RELATORIO DE TELEMETRIA ===\n";
  out << "Registros processados: " << m_data.size() << "\n";
  out << std::fixed << std::setprecision(2);
  out << "Velocidade Media: " << (summary.sampleCount ? summary.sumSpeed / summary.sampleCount : 0.0) << " km/h\n";
  out << "RPM Maximo: " << summary.maxRpm.rpm << " RPM\n";
  out << "Aceleracao Media: " << (summary.accelerationSamples ? summary.sumAcceleration / summary.accelerationSamples : 0.0) << " m/s^2\n";
  out << "Maior lateral_g estimado: " << summary.maxLateralG << " g\n";
  out << "--- ALERTAS CRITICOS ---\n";
  out << "Eventos acima de " << speedLimit << " km/h: " << summary.overspeed << "\n";
  out << "Eventos acima de " << rpmLimit << " RPM: " << summary.highRpm << "\n";
  out << "Frenagens bruscas (< -4.0 m/s^2): " << summary.hardBraking << "\n";
  out << "Eventos de curva (> 0.5 g): " << summary.curveEvents << "\n";
}

} // namespace Telemetry