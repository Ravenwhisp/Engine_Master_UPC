#include "Globals.h"
#include "ImporterMesh.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"

Asset* ImporterMesh::createAssetInstance(const MD5Hash& uid) const
{
    return new MeshAsset(uid);
}

bool ImporterMesh::importNative(const std::filesystem::path& path, MeshAsset* dst)
{
	return false;
}

static uint64_t submeshSerialSize(const Submesh& sm)
{
    return sizeof(uint32_t)                           // indexStart
        + sizeof(uint32_t);                           // indexCount
}

uint64_t ImporterMesh::saveTyped(const MeshAsset* source, uint8_t** outBuffer)
{
    uint64_t size = 0;

    size += sizeof(uint32_t) + source->m_uid.size();         // uid string

    size += sizeof(uint32_t);                                 // vertexCount
    size += source->vertices.size() * sizeof(Vertex);

    size += sizeof(uint32_t);                                 // indexByteCount
    size += sizeof(uint32_t);                                 // indexFormat
    size += source->indices.size();                           // raw index bytes

    size += sizeof(uint32_t);                                 // submeshCount
    for (const Submesh& sm : source->submeshes)
        size += submeshSerialSize(sm);

    size += sizeof(Vector3);                                  // boundsCenter
    size += sizeof(Vector3);                                  // boundsExtents

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.string(source->m_uid);

    writer.u32(static_cast<uint32_t>(source->vertices.size()));
    writer.bytes(source->vertices.data(), source->vertices.size() * sizeof(Vertex));

    writer.u32(static_cast<uint32_t>(source->indices.size()));
    writer.u32(static_cast<uint32_t>(source->indexFormat));
    writer.bytes(source->indices.data(), source->indices.size());

    writer.u32(static_cast<uint32_t>(source->submeshes.size()));
    for (const Submesh& sm : source->submeshes)
    {
        writer.u32(sm.indexStart);
        writer.u32(sm.indexCount);
    }

    writer.bytes(&source->boundsCenter, sizeof(Vector3));
    writer.bytes(&source->boundsExtents, sizeof(Vector3));

    *outBuffer = buffer;
    return size;
}

void ImporterMesh::loadTyped(const uint8_t* buffer, MeshAsset* mesh)
{
    BinaryReader reader(buffer);

    mesh->m_uid = reader.string();

    const uint32_t vertexCount = reader.u32();
    mesh->vertices.resize(vertexCount);
    reader.bytes(mesh->vertices.data(), vertexCount * sizeof(Vertex));

    const uint32_t indexByteCount = reader.u32();
    mesh->indexFormat = static_cast<DXGI_FORMAT>(reader.u32());
    mesh->indices.resize(indexByteCount);
    reader.bytes(mesh->indices.data(), indexByteCount);

    const uint32_t submeshCount = reader.u32();
    mesh->submeshes.resize(submeshCount);
    for (Submesh& sm : mesh->submeshes)
    {
        sm.indexStart = reader.u32();
        sm.indexCount = reader.u32();
    }

    reader.bytes(&mesh->boundsCenter, sizeof(Vector3));
    reader.bytes(&mesh->boundsExtents, sizeof(Vector3));
}
