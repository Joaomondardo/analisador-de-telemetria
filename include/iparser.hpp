#pragma once
#include "telemetry_record.hpp" // Sua struct de dados
#include <string>
#include <vector>


namespace Telemetry {
class IParser {
public:
  virtual ~IParser() = default;
  virtual std::vector<TelemetryRecord> parse(const std::string &source) = 0;
};
} // namespace Telemetry