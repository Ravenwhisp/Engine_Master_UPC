#include "Globals.h"
#include "ImporterRegistry.h"
#include "Importer.h"

void ImporterRegistry::registerImporter(std::unique_ptr<Importer> importer)
{
    m_importersByType[importer->getAssetType()] = importer.get();
    m_importers.push_back(std::move(importer));
}

Importer* ImporterRegistry::findImporter(const std::filesystem::path& filePath) const
{
    for (const auto& importer : m_importers)
    {
        if (importer->canImport(filePath))
            return importer.get();
    }
    return nullptr;
}

Importer* ImporterRegistry::findImporter(AssetType type) const
{
    auto it = m_importersByType.find(type);
    return it != m_importersByType.end() ? it->second : nullptr;
}