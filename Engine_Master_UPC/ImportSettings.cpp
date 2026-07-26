#include "Globals.h"
#include "ImportSettings.h"
#include "TextureImportSettings.h"
#include "NavMeshBuildSettings.h"

std::unique_ptr<ImportSettings> ImportSettings::CreateForType(AssetType type)
{
    switch (type)
    {
    case AssetType::TEXTURE:
        return std::make_unique<TextureImportSettings>();
    case AssetType::NAVMESH:
        return std::make_unique<NavMeshBuildSettings>();
    default:
        return nullptr;
    }
}
