#include "FileSystemModule.h"
#include <filesystem>
#include <fstream>

#include "TextureImporter.h"

bool FileSystemModule::init()
{
    importers.push_back(new TextureImporter());
    return true;
}

unsigned int FileSystemModule::load(const char* filePath, char** buffer) const
{
    if (!filePath || !buffer) return 0;

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) return 0;

    std::streamsize size = file.tellg();
    if (size <= 0) return 0;

    file.seekg(0, std::ios::beg);

    char* data = new char[size];
    if (!file.read(data, size))
    {
        delete[] data;
        return 0;
    }

    *buffer = data;
    return static_cast<unsigned int>(size);
}

unsigned int FileSystemModule::save(const char* filePath, const void* buffer, unsigned int size, bool append) const
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
    if (!file) return 0;

    file.write(static_cast<const char*>(buffer), size);
    if (!file) return 0;

    return size;
}

bool FileSystemModule::copy(const char* sourceFilePath, const char* destinationFilePath) const
{
	return std::filesystem::copy_file(sourceFilePath, destinationFilePath);
}

bool FileSystemModule::deleteFile(const char* filePath) const
{
	return std::filesystem::remove(filePath);
}

bool FileSystemModule::createDirectory(const char* directoryPath) const
{
	return std::filesystem::create_directory(directoryPath);
}

bool FileSystemModule::exists(const char* filePath) const
{
	return std::filesystem::exists(filePath);
}

bool FileSystemModule::isDirectory(const char* path) const
{
	return std::filesystem::is_directory(path);
}
