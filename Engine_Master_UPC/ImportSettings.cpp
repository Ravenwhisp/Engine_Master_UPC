#include "Globals.h"
#include "ImportSettings.h"
#include "TextureImportSettings.h"

std::unique_ptr<ImportSettings> ImportSettings::CreateForType(AssetType type)
{
    switch (type)
    {
    case AssetType::TEXTURE:
        return std::make_unique<TextureImportSettings>();
    default:
        return nullptr;
    }
}
