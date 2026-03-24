#include "Globals.h"
#include "ModuleFileSystem.h"

#include <fstream>
#include <filesystem>


std::vector<uint8_t> ModuleFileSystem::read(const std::filesystem::path& filePath) const
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        DEBUG_ERROR("[ModuleFileSystem] Could not open '%s' for reading.", filePath.string().c_str());
        return {};
    }

    const std::streamsize size = file.tellg();
    if (size <= 0)
    {
        DEBUG_ERROR("[ModuleFileSystem] File '%s' is empty.", filePath.string().c_str());
        return {};
    }

    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        DEBUG_ERROR("[ModuleFileSystem] Failed to read '%s'.", filePath.string().c_str());
        return {};
    }

    return buffer;
}

bool ModuleFileSystem::write(const std::filesystem::path& filePath, const void* data, size_t size, bool append) const
{
    if (!data || size == 0)
    {
        return false;
    }

    std::filesystem::create_directories(filePath.parent_path());

    std::ios::openmode mode = std::ios::binary | std::ios::out;
    mode |= append ? std::ios::app : std::ios::trunc;

    std::ofstream file(filePath, mode);
    if (!file)
    {
        DEBUG_ERROR("[ModuleFileSystem] Could not open '%s' for writing.", filePath.string().c_str());
        return false;
    }

    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (!file)
    {
        DEBUG_ERROR("[ModuleFileSystem] Failed to write to '%s'.", filePath.string().c_str());
        return false;
    }

    return true;
}

bool ModuleFileSystem::copy(const std::filesystem::path& src, const std::filesystem::path& dst) const
{
    std::error_code ec;
    const bool ok = std::filesystem::copy_file(src, dst, ec);
    if (ec)
    {
        DEBUG_ERROR("[ModuleFileSystem] copy '%s' -> '%s': %s", src.string().c_str(), dst.string().c_str(), ec.message().c_str());
    }
    return ok;
}

bool ModuleFileSystem::move(const std::filesystem::path& src, const std::filesystem::path& dst) const
{
    std::error_code ec;
    std::filesystem::rename(src, dst, ec);
    if (ec)
    {
        DEBUG_ERROR("[ModuleFileSystem] move '%s' -> '%s': %s", src.string().c_str(), dst.string().c_str(), ec.message().c_str());
    }
    return ec.value() == 0;
}

bool ModuleFileSystem::remove(const std::filesystem::path& filePath) const
{
    std::error_code ec;
    std::filesystem::remove(filePath, ec);
    if (ec)
        DEBUG_ERROR("[ModuleFileSystem] remove '%s': %s", filePath.string().c_str(), ec.message().c_str());
    return ec.value() == 0;
}

bool ModuleFileSystem::exists(const std::filesystem::path& filePath) const
{
    return std::filesystem::exists(filePath);
}

bool ModuleFileSystem::isDirectory(const std::filesystem::path& path) const
{
    return std::filesystem::is_directory(path);
}

bool ModuleFileSystem::createDirectory(const std::filesystem::path& path) const
{
    std::error_code ec;
    std::filesystem::create_directory(path, ec);
    return ec.value() == 0;
}