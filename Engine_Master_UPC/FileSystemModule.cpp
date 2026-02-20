#include "Globals.h"
#include "FileSystemModule.h"
#include <filesystem>
#include <fstream>

#include "TextureImporter.h"
#include "Asset.h"

bool FileSystemModule::init()
{
    /// TESTING
    auto textureImporter = new TextureImporter();

    TextureAsset skyBox(rand());
    textureImporter->import(L"Assets/Textures/cubemap2.dds", &skyBox);

    uint8_t* buffer = nullptr;
    uint64_t size = textureImporter->save(&skyBox, &buffer);
    save("Library/Textures/cubemap2.asset", buffer, static_cast<unsigned int>(size));

    textureImporter->load(buffer, &skyBox);
    ///

    /// TESTING
    // Image we want to import a .gltf file
    // The final result in Unity is a prefab with the meshes, materials and textures
    // So we need to ask MeshImporter, MaterialImporter and TextureImporter during the pipeline

    importers.push_back(textureImporter);
    return true;
}

Asset* FileSystemModule::import(const char* filePath) const
{
    for (Importer* importer : importers)
	{
		if (importer->canImport(filePath))
		{
			Asset* asset = importer->createAssetInstance();
			if (importer->import(filePath, asset))
			{
				return asset;
			}
			else
			{
				delete asset;
				return nullptr;
			}
		}
	}
    return nullptr;
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
