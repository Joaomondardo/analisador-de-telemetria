#include "telemetry_analyzer.hpp"
#include <algorithm>
#include <atomic>
#include <execution>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <numeric>
#include <stdexcept>

namespace Telemetry {

// Alterado de TelemetryData para TelemetryRecord
TelemetryAnalyzer::TelemetryAnalyzer(const std::vector<TelemetryRecord> &data)
    : m_data(data) {}

double TelemetryAnalyzer::getAverageSpeed() const {
  if (m_data.empty())
    return 0.0;
  double sum = 0.0;
  std::mutex mtx;
  std::for_each(std::execution::par, m_data.begin(), m_data.end(),
                [&](const TelemetryRecord &r) {
                  std::lock_guard<std::mutex> lk(mtx);
                  sum += r.speed;
                });
  return sum / m_data.size();
}

TelemetryRecord TelemetryAnalyzer::getMaxRPM() const {
  if (m_data.empty())
    return TelemetryRecord{};
  TelemetryRecord maxRec = m_data.front();
  std::mutex mtx;
  std::for_each(std::execution::par, m_data.begin(), m_data.end(),
                [&](const TelemetryRecord &r) {
                  std::lock_guard<std::mutex> lk(mtx);
                  if (r.rpm > maxRec.rpm)
                    maxRec = r;
                });
  return maxRec;
}

int TelemetryAnalyzer::countOverspeedEvents(double speedLimit) const {
  std::atomic<int> cnt{0};
  std::for_each(std::execution::par, m_data.begin(), m_data.end(),
                [&](const TelemetryRecord &r) {
                  if (r.speed > speedLimit)
                    cnt.fetch_add(1, std::memory_order_relaxed);
                });
  return cnt.load();
}

int TelemetryAnalyzer::countHighRpmEvents(int32_t rpmLimit) const {
  std::atomic<int> cnt{0};
  std::for_each(std::execution::par, m_data.begin(), m_data.end(),
                [&](const TelemetryRecord &r) {
                  if (r.rpm > rpmLimit)
                    cnt.fetch_add(1, std::memory_order_relaxed);
                });
  return cnt.load();
}

void TelemetryAnalyzer::generateReport(const std::string &filename,
                                       double speedLimit,
                                       int32_t rpmLimit) const {
  std::ofstream out(filename);
  if (!out.is_open())
    throw std::runtime_error("Nao foi possivel criar o arquivo de relatorio: " +
                             filename);

  int overspeed = countOverspeedEvents(speedLimit);
  int highRpm = countHighRpmEvents(rpmLimit);
  TelemetryRecord maxRpm = getMaxRPM();

  out << "=== RELATORIO DE TELEMETRIA ===\n";
  out << "Registros processados: " << m_data.size() << "\n";
  out << std::fixed << std::setprecision(2);
  out << "Velocidade Media: " << getAverageSpeed() << " km/h\n";
  out << "RPM Maximo: " << maxRpm.rpm << " RPM\n";
  out << "--- ALERTAS CRITICOS ---\n";
  out << "Eventos acima de " << speedLimit << " km/h: " << overspeed << "\n";
  out << "Eventos acima de " << rpmLimit << " RPM: " << highRpm << "\n";
}

} // namespace Telemetry