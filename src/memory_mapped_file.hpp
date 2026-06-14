#pragma once

#include <cstddef>
#include <string>
#include <stdexcept>
#include <windows.h>

class MemoryMappedFile {
public:
    explicit MemoryMappedFile(const std::string& path) {
        // Open file for reading
        file_handle_ = CreateFileA(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (file_handle_ == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        LARGE_INTEGER file_size_li;
        if (!GetFileSizeEx(file_handle_, &file_size_li)) {
            CloseHandle(file_handle_);
            throw std::runtime_error("Failed to get file size: " + path);
        }
        size_ = static_cast<size_t>(file_size_li.QuadPart);

        // Create file mapping
        mapping_handle_ = CreateFileMappingA(
            file_handle_,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr);
        if (!mapping_handle_) {
            CloseHandle(file_handle_);
            throw std::runtime_error("Failed to create file mapping: " + path);
        }

        // Map view of file
        data_ = static_cast<const char*>(MapViewOfFile(
            mapping_handle_,
            FILE_MAP_READ,
            0,
            0,
            0));
        if (!data_) {
            CloseHandle(mapping_handle_);
            CloseHandle(file_handle_);
            throw std::runtime_error("Failed to map view of file: " + path);
        }
    }

    ~MemoryMappedFile() {
        if (data_) {
            UnmapViewOfFile(data_);
        }
        if (mapping_handle_) {
            CloseHandle(mapping_handle_);
        }
        if (file_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_handle_);
        }
    }

    const char* data() const noexcept { return data_; }
    size_t size() const noexcept { return size_; }

    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

private:
    HANDLE file_handle_ = INVALID_HANDLE_VALUE;
    HANDLE mapping_handle_ = nullptr;
    const char* data_ = nullptr;
    size_t size_ = 0;
};
