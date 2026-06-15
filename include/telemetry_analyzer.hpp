#pragma once

#include "telemetry_record.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace Telemetry {

class TelemetryAnalyzer {
public:
  explicit TelemetryAnalyzer(const std::vector<TelemetryRecord> &data);

  double getAverageSpeed() const;
  TelemetryRecord getMaxRPM() const;
  int countOverspeedEvents(double speedLimit) const;
  int countHighRpmEvents(int32_t rpmLimit) const;
  int countHardBrakingEvents(double threshold = -4.0) const;
  int countCurveEvents(double lateralGThreshold = 0.5) const;
  double getAverageAcceleration() const;
  double getMaxLateralG() const;
  void generateReport(const std::string &filename, double speedLimit = 100.0,
                      int32_t rpmLimit = 3000) const;
  void exportToJSON(const std::string &filename, double speedLimit = 100.0,
                    int32_t rpmLimit = 3000) const;
  // Overload that accepts optional custom channel info to include in JSON
  void exportToJSON(const std::string &filename,
                    double speedLimit,
                    int32_t rpmLimit,
                    const std::string &customFormula,
                    double customMean,
                    double customMax) const;
  void exportToCSV(const std::string &filename, double speedLimit = 100.0,
                   int32_t rpmLimit = 3000) const;

private:
  struct DerivedSummary {
    double sumSpeed = 0.0;
    double sumAcceleration = 0.0;
    double maxLateralG = 0.0;
    int overspeed = 0;
    int highRpm = 0;
    int hardBraking = 0;
    int curveEvents = 0;
    size_t sampleCount = 0;
    size_t accelerationSamples = 0;
    TelemetryRecord maxRpm{};
    bool hasMaxRpm = false;
  };

  static DerivedSummary combineSummaries(const DerivedSummary &left,
                                        const DerivedSummary &right);
  static DerivedSummary processChunk(const std::vector<TelemetryRecord> &data,
                                     size_t first,
                                     size_t last,
                                     const TelemetryRecord *previous,
                                     double speedLimit,
                                     int32_t rpmLimit,
                                     double brakingThreshold,
                                     double curveGThreshold);
  DerivedSummary computeDerivedSummary(double speedLimit,
                                       int32_t rpmLimit,
                                       double brakingThreshold,
                                       double curveGThreshold) const;

  const std::vector<TelemetryRecord> &m_data;
};

} // namespace Telemetry