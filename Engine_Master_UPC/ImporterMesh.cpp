#include "ImporterMesh.h"
#include "MeshAsset.h"

Asset* ImporterMesh::createAssetInstance(const MD5Hash& uid) const

{
    return new MeshAsset(uid);
}
bool ImporterMesh::importNative(const std::filesystem::path& path, MeshAsset* dst)
{
	return false;
}

uint64_t ImporterMesh::saveTyped(const MeshAsset* source, uint8_t** outBuffer)
{
    uint64_t size = 0;

    size += sizeof(uint64_t); // uid

    size += sizeof(uint32_t); // vertexCount
    size += source->vertices.size() * sizeof(Vertex);

    size += sizeof(uint32_t); // indexCount
    size += sizeof(uint32_t); // indexFormat
    size += source->indices.size() * sizeof(uint8_t);

    size += sizeof(uint32_t); // submeshCount
    size += source->submeshes.size() * sizeof(Submesh);

    size += sizeof(Vector3); // boundsCenter
    size += sizeof(Vector3); // boundsExtents

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.string(source->m_uid);

    writer.u32(static_cast<uint32_t>(source->vertices.size()));
    writer.bytes(source->vertices.data(), source->vertices.size() * sizeof(Vertex));

    writer.u32(static_cast<uint32_t>(source->indices.size()));
    writer.u32(static_cast<uint32_t>(source->indexFormat));
    writer.bytes(source->indices.data(), source->indices.size());

    writer.u32(static_cast<uint32_t>(source->submeshes.size()));
    writer.bytes(source->submeshes.data(), source->submeshes.size() * sizeof(Submesh));

    writer.bytes(&source->boundsCenter, sizeof(Vector3));
    writer.bytes(&source->boundsExtents, sizeof(Vector3));

    *outBuffer = buffer;
    return size;
}

void ImporterMesh::loadTyped(const uint8_t* buffer, MeshAsset* mesh)
{
    BinaryReader reader(buffer);

    mesh->m_uid = reader.u64();

    uint32_t vertexCount = reader.u32();
    mesh->vertices.resize(vertexCount);
    reader.bytes(mesh->vertices.data(), vertexCount * sizeof(Vertex));

    uint32_t indexCount = reader.u32();
    mesh->indexFormat = static_cast<DXGI_FORMAT>(reader.u32());
    mesh->indices.resize(indexCount);
    reader.bytes(mesh->indices.data(), indexCount);

    uint32_t submeshCount = reader.u32();
    mesh->submeshes.resize(submeshCount);
    reader.bytes(mesh->submeshes.data(), submeshCount * sizeof(Submesh));

    reader.bytes(&mesh->boundsCenter, sizeof(Vector3));
    reader.bytes(&mesh->boundsExtents, sizeof(Vector3));
}
