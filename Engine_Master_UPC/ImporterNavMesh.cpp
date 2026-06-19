#include "ImporterNavMesh.h"
#include "NavMeshAsset.h"
#include "BinaryArchive.h"
#include <cstring>

bool ImporterNavMesh::canImport(const std::filesystem::path& path) const
{
    return false;
}

Asset* ImporterNavMesh::createAssetInstance(AssetReference& uid) const
{
    return new NavMeshAsset(uid);
}

AssetType ImporterNavMesh::getAssetType() const
{
    return AssetType::NAVMESH;
}

bool ImporterNavMesh::saveNative(const Asset* asset, const std::filesystem::path& path)
{
    return false;
}

bool ImporterNavMesh::import(const std::filesystem::path& sourcePath, Asset* outAsset)
{
    return false;
}

uint64_t ImporterNavMesh::save(const Asset* asset, uint8_t** outBuffer)
{
    BinaryArchive archive(ArchiveMode::Output);
    const_cast<Asset*>(asset)->serialize(archive);
    const size_t sz = archive.size();
    uint8_t* buf = new uint8_t[sz];
    std::memcpy(buf, archive.data(), sz);
    *outBuffer = buf;
    return sz;
}

void ImporterNavMesh::load(const uint8_t* buffer, Asset* outAsset)
{
    BinaryArchive archive(buffer, ArchiveMode::Input);
    outAsset->serialize(archive);
}
