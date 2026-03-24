#pragma once
#include "Module.h"
#include <filesystem>
#include <vector>
#include <cstdint>
#include <cstddef>

// Pure file-system I/O module.
class ModuleFileSystem : public Module
{
public:
    bool init() override;

    std::vector<uint8_t> read(const std::filesystem::path& filePath) const;

    bool write(const std::filesystem::path& filePath, const void* data, size_t size, bool append = false) const;

    bool copy(const std::filesystem::path& src, const std::filesystem::path& dst)  const;
    bool move(const std::filesystem::path& src, const std::filesystem::path& dst)  const;
    bool remove(const std::filesystem::path& filePath)                             const;
    bool exists(const std::filesystem::path& filePath)                             const;
    bool isDirectory(const std::filesystem::path& path)                            const;
    bool createDirectory(const std::filesystem::path& path)                        const;
};