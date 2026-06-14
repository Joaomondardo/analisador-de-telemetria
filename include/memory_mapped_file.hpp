#pragma once

#include <cstddef>
#include <cstring>

#include <string>
#include <windows.h>

namespace Telemetry {

/**
 * RAII wrapper for a read‑only memory‑mapped file on Windows.
 * It opens the file for reading, creates a file mapping, maps the whole view,
 * and provides const access to the data. All resources are released in the
 * destructor.
 */
class MemoryMappedFile {
public:
    explicit MemoryMappedFile(const std::string& path);
    ~MemoryMappedFile();

    const char* data() const noexcept { return data_; }
    std::size_t size() const noexcept { return size_; }

private:
    HANDLE fileHandle_{ INVALID_HANDLE_VALUE };
    HANDLE mappingHandle_{ nullptr };
    const char* data_{ nullptr };
    std::size_t size_{ 0 };
};

} // namespace Telemetry
