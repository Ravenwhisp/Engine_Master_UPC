#pragma once

#include "Asset.h"
#include "Importer.h"

class ImporterRegistry {
public:
    void      registerImporter(std::unique_ptr<Importer> importer);

    Importer* findImporter(const std::filesystem::path& filePath) const;
    Importer* findImporter(AssetType type)                        const;

private:
    std::unordered_map<AssetType, Importer*> m_importersByType;
    std::vector<std::unique_ptr<Importer>>   m_importers;
};
