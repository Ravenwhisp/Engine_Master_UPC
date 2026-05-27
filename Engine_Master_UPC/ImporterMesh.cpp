#include "Globals.h"
#include "ImporterMesh.h"

Asset* ImporterMesh::createAssetInstance(AssetReference& uid) const
{
    return new MeshAsset(uid);
}

bool ImporterMesh::saveNative(const MeshAsset* asset, const std::filesystem::path& path)
{
    return false;
}

bool ImporterMesh::importNative(const std::filesystem::path& path, MeshAsset* dst)
{
	return false;
}

