#pragma once
#include "telemetry_data.hpp"
#include <string>
#include <vector>


namespace Telemetry {

class TelemetryAnalyzer {
private:
  const std::vector<TelemetryData> &m_data;

public:
  explicit TelemetryAnalyzer(const std::vector<TelemetryData> &data);
  double getAverageSpeed() const;
  TelemetryData getMaxRPM() const;
  int countOverspeedEvents(double speedLimit) const;
  int countHighRpmEvents(int32_t rpmLimit) const;
  void generateReport(const std::string& filename, double speedLimit = 100.0, int32_t rpmLimit = 3000) const;
};

} // namespace Telemetry