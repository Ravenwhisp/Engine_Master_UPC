#include "Globals.h"

#include "Application.h"
#include "ModuleFileSystem.h"
#include <filesystem>
#include <fstream>

#include "TextureImporter.h"
#include "ModelImporter.h"
#include "FontImporter.h"
#include "Asset.h"

#include "TextureAsset.h"
#include "ModuleAssets.h"

#include "FileIO.h"
#include "ImporterRegistry.h"
#include "MetadataStore.h"
#include "AssetScanner.h"
#include "ContentRegistry.h"

#include "UID.h"

bool ModuleFileSystem::init()
{
    m_fileIO = std::make_unique<FileIO>();
    m_metadataStore = std::make_unique<MetadataStore>();
    m_importerRegistry = std::make_unique<ImporterRegistry>();

    m_importerRegistry->registerImporter(std::make_unique<TextureImporter>());
    m_importerRegistry->registerImporter(std::make_unique<ModelImporter>());
    m_importerRegistry->registerImporter(std::make_unique<FontImporter>());

    m_scanner = std::make_unique<AssetScanner>(m_fileIO.get(), m_metadataStore.get(), m_importerRegistry.get());
    m_contentRegistry = std::make_unique<ContentRegistry>(m_fileIO.get());

    return true;
}

AssetMetadata* ModuleFileSystem::getMetadata(UID uid)
{
    return m_metadataStore->getMetadata(uid);
}

UID ModuleFileSystem::findByPath(const std::filesystem::path& sourcePath) const
{
    return m_metadataStore->findByPath(sourcePath);
}

void ModuleFileSystem::registerMetadata(const AssetMetadata& meta, const std::filesystem::path& sourcePath)
{
    m_metadataStore->registerMetadata(meta, sourcePath);
}

unsigned int ModuleFileSystem::load(const std::filesystem::path& filePath, char** buffer) const
{
    return m_fileIO->load(filePath, buffer);
}

unsigned int ModuleFileSystem::load(const char* filePath, char** buffer) const
{
    return m_fileIO->load(filePath, buffer);
}

unsigned int ModuleFileSystem::save(const std::filesystem::path& filePath, const void* buffer, unsigned int size, bool append) const
{
    return m_fileIO->save(filePath, buffer, size, append);
}

unsigned int ModuleFileSystem::save(const char* filePath, const void* buffer, unsigned int size, bool append) const
{
    return m_fileIO->save(filePath, buffer, size, append);
}

bool ModuleFileSystem::copy(const char* sourceFilePath, const char* destinationFilePath) const
{
    return m_fileIO->copy(sourceFilePath, destinationFilePath);
}

bool ModuleFileSystem::move(const char* sourceFilePath, const char* destinationFilePath) const
{
    return m_fileIO->move(sourceFilePath, destinationFilePath);
}

bool ModuleFileSystem::deleteFile(const char* filePath) const
{
    return m_fileIO->deleteFile(filePath);
}

bool ModuleFileSystem::createDirectory(const char* directoryPath) const
{
	return m_fileIO->createDirectory(directoryPath);
}

bool ModuleFileSystem::exists(const char* filePath) const
{
	return m_fileIO->exists(filePath);
}

bool ModuleFileSystem::isDirectory(const char* path) const
{
	return m_fileIO->isDirectory(path);
}

Importer* ModuleFileSystem::findImporter(const std::filesystem::path& filePath) const
{
    return m_importerRegistry->findImporter(filePath);
}

Importer* ModuleFileSystem::findImporter(const char* filePath) const
{
    return m_importerRegistry->findImporter(filePath);
}

Importer* ModuleFileSystem::findImporter(AssetType type) const
{
    return m_importerRegistry->findImporter(type);
}

void ModuleFileSystem::rebuild()
{
    std::string rootStr = ASSETS_FOLDER;
    if (!rootStr.empty() && (rootStr.back() == '/' || rootStr.back() == '\\'))
    {
        rootStr.pop_back();
    }

    const std::filesystem::path root = rootStr;

    m_scanner->scan(root);
    m_contentRegistry->rebuild(root);
}

std::shared_ptr<FileEntry> ModuleFileSystem::getRoot() const
{
    return m_contentRegistry->getRoot();
}

std::shared_ptr<FileEntry> ModuleFileSystem::getEntry(const std::filesystem::path& path) const
{
    return m_contentRegistry->getEntry(path);
}