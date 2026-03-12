#pragma once

#include "Asset.h"
class Importer;

class ImporterRegistry {
public:

    virtual void registerImporter(std::unique_ptr<Importer> importer);
    Importer* findImporter(const std::filesystem::path& filePath) const;
    Importer* findImporter(const char* filePath) const;
    Importer* findImporter(const AssetType type);

private:
    std::unordered_map<AssetType, Importer*>    m_importersByType;
    std::vector<std::unique_ptr<Importer>> m_importers;
};
