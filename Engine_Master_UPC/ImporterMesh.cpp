#include "Globals.h"
#include "ImporterMesh.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"

Asset* ImporterMesh::createAssetInstance(const UID& uid) const
{
    return new MeshAsset(uid);
}

bool ImporterMesh::importNative(const std::filesystem::path& path, MeshAsset* dst)
{
	return false;
}

bool ImporterMesh::saveNative(const std::filesystem::path& path, const MeshAsset* src)
{
    return false;
}

uint64_t ImporterMesh::saveTyped(const MeshAsset* source, uint8_t** outBuffer)
{
    return CerealUtils::saveTo(*source, outBuffer);
}

void ImporterMesh::loadTyped(const uint8_t* buffer, MeshAsset* mesh)
{
    CerealUtils::loadFrom(buffer, *mesh);
}
