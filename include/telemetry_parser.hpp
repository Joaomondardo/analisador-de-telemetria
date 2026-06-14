#pragma once

#include "telemetry_data.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace Telemetry {

class TelemetryParser {
public:
  // Load telemetry data from a CSV file path.
  std::vector<TelemetryData> loadFromCSV(const std::string &filepath) const;
  // Load telemetry data from a memory buffer (zero‑copy).
  std::vector<TelemetryData> loadFromBuffer(std::string_view buffer) const;
};

} // namespace Telemetry
