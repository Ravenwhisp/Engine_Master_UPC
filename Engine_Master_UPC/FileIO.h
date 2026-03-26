#pragma once
#include "Module.h"
#include <filesystem>
#include <vector>
#include <cstdint>
#include <cstddef>

class FileIO
{
public:

    static std::vector<uint8_t> read(const std::filesystem::path& filePath);

    static bool write(const std::filesystem::path& filePath, const void* data, size_t size, bool append = false);

    static bool copy(const std::filesystem::path& src, const std::filesystem::path& dst);
    static bool move(const std::filesystem::path& src, const std::filesystem::path& dst);
    static bool remove(const std::filesystem::path& filePath);
    static bool exists(const std::filesystem::path& filePath);
    static bool isDirectory(const std::filesystem::path& path);
    static bool createDirectory(const std::filesystem::path& path);
};