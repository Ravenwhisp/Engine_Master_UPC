#include "Globals.h"
#include <filesystem>
#include "ImporterRegistry.h"
#include "Importer.h"
#include "Asset.h"

void ImporterRegistry::registerImporter(std::unique_ptr<Importer> importer)
{
    m_importersByType[importer->getAssetType()] = importer.get();
    m_importers.push_back(std::move(importer));
}

Importer* ImporterRegistry::findImporter(const std::filesystem::path& filePath) const
{
    return findImporter(filePath.string().c_str());
}

Importer* ImporterRegistry::findImporter(const char* filePath) const
{
    for (const auto& importer : m_importers)
    {
        if (importer->canImport(filePath))
        {
            return importer.get();
        }
    }
    return nullptr;
}

Importer* ImporterRegistry::findImporter(const AssetType type)
{
    auto it = m_importersByType.find(type);
    if (it != m_importersByType.end())
    {
        return it->second;
    }
    return nullptr;
}
