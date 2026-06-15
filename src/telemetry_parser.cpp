#define NOMINMAX
#include <algorithm>
#include "telemetry_parser.hpp"
#include "memory_mapped_file.hpp"
#include <array>
#include <charconv>
#include <cstring>
#include <future>
#include <immintrin.h>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace Telemetry {

namespace {

// Helper: Parse double using std::from_chars (branchless, no allocations)
inline bool parseDouble(std::string_view str, double& result) {
  auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
  return ec == std::errc();
}

// Helper: Parse int32_t using std::from_chars
inline bool parseInt32(std::string_view str, int32_t& result) {
  auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
  return ec == std::errc();
}

// Helper: Parse uint64_t using std::from_chars
inline bool parseUInt64(std::string_view str, uint64_t& result) {
  auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
  return ec == std::errc();
}

// Portable trailing-zero count helper
inline unsigned int countTrailingZeros(unsigned int mask) {
#if defined(_MSC_VER)
  return static_cast<unsigned int>(_tzcnt_u32(mask));
#else
  return static_cast<unsigned int>(__builtin_ctz(mask));
#endif
}

// Find the next newline in the buffer between [pos, end)
// Returns true if found and sets newlinePos, otherwise returns false.
inline bool findNextNewline(std::string_view buffer,
                            size_t pos,
                            size_t end,
                            size_t &newlinePos) {
  const char *data = buffer.data();
  size_t limit = std::min(end, buffer.size());

  while (pos + 32 <= limit) {
    __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data + pos));
    __m256i needle = _mm256_set1_epi8('\n');
    __m256i eq = _mm256_cmpeq_epi8(chunk, needle);
    unsigned int mask = static_cast<unsigned int>(_mm256_movemask_epi8(eq));
    if (mask) {
      unsigned int idx = countTrailingZeros(mask);
      newlinePos = pos + idx;
      return true;
    }
    pos += 32;
  }

  while (pos < limit) {
    if (data[pos] == '\n') {
      newlinePos = pos;
      return true;
    }
    ++pos;
  }

  return false;
}

// Process a chunk of the buffer, returning parsed records
// This function is designed to be called by worker threads
std::vector<TelemetryRecord> processChunk(
    std::string_view buffer,
    size_t chunkStart,
    size_t chunkEnd) {
  std::vector<TelemetryRecord> localRecords;
  
  size_t pos = chunkStart;
  
  // If chunk starts in the middle of a line, advance to the next line boundary.
  if (pos > 0 && pos < buffer.size() && buffer[pos - 1] != '\n') {
    size_t nextLineStart;
    if (findNextNewline(buffer, pos, buffer.size(), nextLineStart)) {
      pos = nextLineStart + 1;
    } else {
      return localRecords;
    }
  }
  
  while (pos < chunkEnd && pos < buffer.size()) {
    size_t lineEnd;
    bool found = findNextNewline(buffer, pos, chunkEnd, lineEnd);
    if (!found) {
      if (chunkEnd >= buffer.size()) {
        lineEnd = buffer.size();
      } else {
        break; // partial line at chunk boundary, leave for next thread
      }
    }

    if (pos >= chunkEnd) {
      break;
    }

    std::string_view line = buffer.substr(pos, lineEnd - pos);
    pos = (found ? lineEnd + 1 : lineEnd);
    
    // Trim carriage return if present
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    
    if (line.empty()) {
      continue;
    }
    
    // Split into three fields without allocating strings
    std::array<std::string_view, 3> fields{};
    size_t start = 0;
    size_t fieldIdx = 0;
    
    while (fieldIdx < 3) {
      size_t comma = line.find(',', start);
      if (comma == std::string_view::npos) {
        fields[fieldIdx++] = line.substr(start);
        break;
      }
      fields[fieldIdx++] = line.substr(start, comma - start);
      start = comma + 1;
    }
    
    // Branchless validation: skip if not enough fields
    if (fieldIdx < 3) {
      continue;
    }
    
    // Parse using std::from_chars (no allocations, no exception overhead)
    uint64_t timestamp = 0;
    double speed = 0.0;
    int32_t rpm = 0;
    
    if (parseUInt64(fields[0], timestamp) &&
        parseDouble(fields[1], speed) &&
        parseInt32(fields[2], rpm)) {
      TelemetryRecord entry;
      entry.timestamp = timestamp * 1000;
      entry.speed = speed;
      entry.rpm = rpm;
      localRecords.push_back(entry);
    }
  }
  
  return localRecords;
}

} // anonymous namespace

// Load CSV by memory‑mapping the file and delegating to buffer parser
std::vector<TelemetryRecord>
TelemetryParser::loadFromCSV(const std::string &filepath) const {
  MemoryMappedFile mmf(filepath);
  return loadFromBuffer(std::string_view(mmf.data(), mmf.size()));
}

// Optimized parallel parsing with std::from_chars and divide-and-conquer
std::vector<TelemetryRecord>
TelemetryParser::loadFromBuffer(std::string_view buffer) const {
  if (buffer.empty()) {
    return {};
  }
  
  // Skip header line
  size_t dataStart = 0;
  size_t headerEnd = buffer.find('\n', 0);
  if (headerEnd != std::string_view::npos) {
    dataStart = headerEnd + 1;
  }
  
  if (dataStart >= buffer.size()) {
    return {};
  }
  
  // Determine number of threads (avoid oversubscription)
  unsigned int numThreads = std::thread::hardware_concurrency();
  if (numThreads == 0) {
    numThreads = 4; // Fallback
  }
  numThreads = std::min(numThreads, 16U); // Cap at 16 to avoid excessive overhead
  
  // Calculate chunk size
  size_t totalSize = buffer.size() - dataStart;
  size_t chunkSize = (totalSize + numThreads - 1) / numThreads;
  
  // Launch async tasks for parallel chunk processing
  std::vector<std::future<std::vector<TelemetryRecord>>> futures;
  futures.reserve(numThreads);
  
  for (unsigned int i = 0; i < numThreads; ++i) {
    size_t chunkStart = dataStart + i * chunkSize;
    size_t chunkEnd = (i == numThreads - 1) ? buffer.size() : dataStart + (i + 1) * chunkSize;
    
    futures.emplace_back(std::async(std::launch::async, [buffer, chunkStart, chunkEnd]() {
      return processChunk(buffer, chunkStart, chunkEnd);
    }));
  }
  
  // Aggregate results from all threads (branchless final assembly)
  std::vector<TelemetryRecord> result;
  for (auto& future : futures) {
    auto chunkRecords = future.get();
    result.insert(result.end(), 
                  std::make_move_iterator(chunkRecords.begin()),
                  std::make_move_iterator(chunkRecords.end()));
  }
  
  return result;
}

} // namespace Telemetry