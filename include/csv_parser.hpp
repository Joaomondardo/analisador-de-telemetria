#pragma once

#include "iparser.hpp"
#include "telemetry_parser.hpp"

namespace Telemetry {

// CsvParser parses a CSV file using MemoryMappedFile and returns TelemetryRecord vector.
class CsvParser : public IParser {
public:
    // Parses the CSV file at the given path and returns a vector of TelemetryRecord.
    std::vector<TelemetryRecord> parse(const std::string& source) override;
};

} // namespace Telemetry
