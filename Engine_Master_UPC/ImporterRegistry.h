#pragma once

#include "Importer.h"

class ImporterRegistry 
{
public:
    void      registerImporter(Importer* importer);

    Importer* findImporter(const std::filesystem::path& filePath) const;
    Importer* findImporter(AssetType type)                        const;

private:
    std::unordered_map<AssetType, Importer*> m_importersByType;
    std::vector<Importer*>   m_importers;
};
