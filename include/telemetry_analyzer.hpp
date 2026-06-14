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
  void generateReport(const std::string &filename, double speedLimit = 100.0,
                      int32_t rpmLimit = 3000) const;

private:
  const std::vector<TelemetryRecord> &m_data;
};

} // namespace Telemetry