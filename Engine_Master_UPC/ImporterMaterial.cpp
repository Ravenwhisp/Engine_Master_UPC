#include "Globals.h"
#include "ImporterMaterial.h"

#include "MaterialAsset.h"
Asset* ImporterMaterial::createAssetInstance(AssetReference& uid) const

{
    return new MaterialAsset(uid);
}

bool ImporterMaterial::saveNative(const MaterialAsset* asset, const std::filesystem::path& path)
{
    return false;
}


bool ImporterMaterial::importNative(const std::filesystem::path& path, MaterialAsset* dst)
{
	return false;
}
