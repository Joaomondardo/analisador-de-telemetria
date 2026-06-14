#pragma once

#include <cstddef>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace Telemetry {

/**
 * RAII wrapper for a read‑only memory‑mapped file.
 * Provides a Windows implementation (Win32 API) and a POSIX implementation
 * (mmap).
 */
class MemoryMappedFile {
public:
  explicit MemoryMappedFile(const std::string &path);
  ~MemoryMappedFile();

  const char *data() const noexcept { return data_; }
  std::size_t size() const noexcept { return size_; }

private:
#ifdef _WIN32
  HANDLE fileHandle_{INVALID_HANDLE_VALUE};
  HANDLE mappingHandle_{nullptr};
#else
  int fd_{-1};
#endif
  const char *data_{nullptr};
  std::size_t size_{0};
};

} // namespace Telemetry
