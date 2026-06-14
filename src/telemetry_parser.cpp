#include "telemetry_parser.hpp"
#include "memory_mapped_file.hpp"
#include <array>
#include <string>
#include <vector>

namespace Telemetry {

// Load CSV by memory‑mapping the file and delegating to buffer parser
std::vector<TelemetryData>
TelemetryParser::loadFromCSV(const std::string &filepath) const {
  MemoryMappedFile mmf(filepath);
  return loadFromBuffer(std::string_view(mmf.data(), mmf.size()));
}

// Zero‑copy parsing from a memory buffer
std::vector<TelemetryData>
TelemetryParser::loadFromBuffer(std::string_view buffer) const {
  std::vector<TelemetryData> data;
  size_t pos = 0;
  // Skip header line
  size_t eol = buffer.find('\n', pos);
  if (eol != std::string_view::npos)
    pos = eol + 1;
  while (pos < buffer.size()) {
    size_t line_end = buffer.find('\n', pos);
    if (line_end == std::string_view::npos)
      line_end = buffer.size();
    std::string_view line = buffer.substr(pos, line_end - pos);
    pos = line_end + 1;
    if (line.empty())
      continue;
    // Trim possible carriage return
    if (!line.empty() && line.back() == '\r')
      line.remove_suffix(1);
    // Split into three fields without allocating strings
    std::array<std::string_view, 3> fields{{}};
    size_t start = 0;
    size_t idx = 0;
    while (idx < 3) {
      size_t comma = line.find(',', start);
      if (comma == std::string_view::npos) {
        fields[idx++] = line.substr(start);
        break;
      }
      fields[idx++] = line.substr(start, comma - start);
      start = comma + 1;
    }
    if (idx < 3)
      continue; // malformed line
    try {
      TelemetryData entry;
      entry.timestamp =
          static_cast<uint64_t>(std::stod(std::string(fields[0])) * 1000.0);
      entry.speed = std::stod(std::string(fields[1]));
      entry.rpm = static_cast<int32_t>(std::stoi(std::string(fields[2])));
      data.push_back(entry);
    } catch (...) {
      // ignore malformed line
    }
  }
  return data;
}

} // namespace Telemetry