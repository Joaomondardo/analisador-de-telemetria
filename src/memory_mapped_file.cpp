#include "memory_mapped_file.hpp"
#include <stdexcept>

namespace Telemetry {

MemoryMappedFile::MemoryMappedFile(const std::string& path) {
    fileHandle_ = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (fileHandle_ == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle_, &fileSize)) {
        CloseHandle(fileHandle_);
        throw std::runtime_error("Failed to get file size: " + path);
    }
    size_ = static_cast<std::size_t>(fileSize.QuadPart);
    if (size_ == 0) {
        CloseHandle(fileHandle_);
        throw std::runtime_error("File is empty: " + path);
    }

    mappingHandle_ = CreateFileMappingA(
        fileHandle_,
        nullptr,
        PAGE_READONLY,
        0,
        0,
        nullptr);
    if (!mappingHandle_) {
        CloseHandle(fileHandle_);
        throw std::runtime_error("Failed to create file mapping: " + path);
    }

    data_ = static_cast<const char*>(MapViewOfFile(
        mappingHandle_,
        FILE_MAP_READ,
        0,
        0,
        0));
    if (!data_) {
        CloseHandle(mappingHandle_);
        CloseHandle(fileHandle_);
        throw std::runtime_error("Failed to map view of file: " + path);
    }
}

MemoryMappedFile::~MemoryMappedFile() {
    if (data_) {
        UnmapViewOfFile(data_);
        data_ = nullptr;
    }
    if (mappingHandle_) {
        CloseHandle(mappingHandle_);
        mappingHandle_ = nullptr;
    }
    if (fileHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle_);
        fileHandle_ = INVALID_HANDLE_VALUE;
    }
    size_ = 0;
}

} // namespace Telemetry
