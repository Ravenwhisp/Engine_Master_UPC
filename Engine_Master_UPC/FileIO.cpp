#include "Globals.h"
#include "FileIO.h"

#include <filesystem>
#include <fstream>

unsigned int FileIO::load(const std::filesystem::path& filePath, char** buffer) const
{
    std::string pathStr = filePath.string();
    const char* cpath = pathStr.c_str();
    return load(cpath, buffer);
}

unsigned int FileIO::load(const char* filePath, char** buffer) const
{
    if (!filePath || !buffer)
    {
        DEBUG_ERROR("[FileSystemModule] No path or buffer correctly provided.");
        return 0;
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        DEBUG_ERROR("[FileSystemModule] Couldn't open the file while trying to load.");
        return 0;
    }

    std::streamsize size = file.tellg();
    if (size <= 0)
    {
        DEBUG_ERROR("[FileSystemModule] Couldn't load the file since it's empty.");
        return 0;
    }

    file.seekg(0, std::ios::beg);

    char* data = new char[size];
    if (!file.read(data, size))
    {
        DEBUG_ERROR("[FileSystemModule] Couldn't read the file.");
        delete[] data;
        return 0;
    }

    *buffer = data;
    return static_cast<unsigned int>(size);
}

unsigned int FileIO::save(const std::filesystem::path& filePath, const void* buffer, unsigned int size, bool append) const
{
    std::string pathStr = filePath.string();
    const char* cpath = pathStr.c_str();
    return save(cpath, buffer, size, append);
}

unsigned int FileIO::save(const char* filePath, const void* buffer, unsigned int size, bool append) const
{
    if (!filePath || !buffer || size == 0) return 0;

    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());

    std::ios::openmode mode = std::ios::binary | std::ios::out;
    if (append)
    {
        mode |= std::ios::app;
    }
    else
    {
        mode |= std::ios::trunc;

    }

    std::ofstream file(filePath, mode);
    if (!file)
    {
        DEBUG_ERROR("[FileSystemModule] Error while trying to save a file that doesn't exists.");
        return 0;
    }

    file.write(static_cast<const char*>(buffer), size);
    if (!file)
    {
        DEBUG_ERROR("[FileSystemModule] Error while writing into a file.");
        return 0;
    }

    return size;
}

bool FileIO::copy(const char* sourceFilePath, const char* destinationFilePath) const
{
    return std::filesystem::copy_file(sourceFilePath, destinationFilePath);
}

bool FileIO::move(const char* sourceFilePath, const char* destinationFilePath) const
{
    std::error_code error;
    std::filesystem::rename(sourceFilePath, destinationFilePath, error);
    return error.value() == 0;
}

bool FileIO::deleteFile(const char* filePath) const
{
    return std::filesystem::remove(filePath);
}

bool FileIO::createDirectory(const char* directoryPath) const
{
    return std::filesystem::create_directory(directoryPath);
}

bool FileIO::exists(const char* filePath) const
{
    return std::filesystem::exists(filePath);
}

bool FileIO::isDirectory(const char* path) const
{
    return std::filesystem::is_directory(path);
}
