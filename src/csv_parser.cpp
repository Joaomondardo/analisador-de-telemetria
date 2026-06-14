#include "csv_parser.hpp"
#include "telemetry_parser.hpp"
#include <string>
#include <vector>

namespace Telemetry {

std::vector<TelemetryRecord> CsvParser::parse(const std::string &source) {
    // Reuse existing TelemetryParser which already handles memory‑mapped CSV parsing
    TelemetryParser parser;
    return parser.loadFromCSV(source);
}

} // namespace Telemetry