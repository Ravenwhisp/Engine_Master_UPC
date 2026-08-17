#pragma once
#include "Importer.h"
#include <filesystem>
#include <memory>
#include <vector>
#include "AssetType.h"

class ImporterGltf;

class ImporterRegistry
{
public:
    ImporterRegistry();
    ~ImporterRegistry();

    ImporterRegistry(const ImporterRegistry&) = delete;
    ImporterRegistry& operator=(const ImporterRegistry&) = delete;

    Importer* findByPath(const std::filesystem::path& filePath) const;
    Importer* findByType(AssetType type) const;
    bool canImport(const std::filesystem::path& sourcePath) const;

    ImporterGltf* getGltfImporter() const { return m_importerGltfPtr; }

private:
    std::vector<std::unique_ptr<Importer>> m_importers;
    ImporterGltf* m_importerGltfPtr = nullptr;
};
