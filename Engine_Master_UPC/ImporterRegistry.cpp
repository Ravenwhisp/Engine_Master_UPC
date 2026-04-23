#include "Globals.h"
#include "ImporterRegistry.h"
#include "Importer.h"

void ImporterRegistry::registerImporter(Importer* importer)
{
    m_importersByType[importer->getAssetType()] = importer;
    m_importers.push_back(importer);
}

Importer* ImporterRegistry::findImporter(const std::filesystem::path& filePath) const
{
    for (Importer* importer : m_importers)
    {
        if (importer->canImport(filePath))
            return importer;
    }
    return nullptr;
}

Importer* ImporterRegistry::findImporter(AssetType type) const
{
    auto it = m_importersByType.find(type);
    return it != m_importersByType.end() ? it->second : nullptr;
}