#pragma once

#include "Asset.h"
#include "Vertex.h"

#include <cstdint>
#include <vector>

struct Submesh
{
    uint32_t indexStart = 0;
    uint32_t indexCount = 0;
};

class MeshAsset : public Asset
{
public:
    friend class ImporterGltf;

	MeshAsset() {}
	MeshAsset(AssetReference& id) : Asset(id, AssetType::MESH) {}

    const void* getVertexData() const { return vertices.data(); }

    std::vector<Vector3> getVerticesPositions() const;

    uint32_t getVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
    uint32_t getVertexStride() const { return sizeof(Vertex); }

    const void* getIndexData() const { return indices.data(); }
    std::vector<uint8_t> getIndexDataVector() const { return indices; }
    uint32_t getIndexBufferSize() const { return static_cast<uint32_t>(indices.size()); }
    uint32_t getIndexCount() const;

    DXGI_FORMAT getIndexFormat() const { return indexFormat; }

    const std::vector<Submesh>& getSubmeshes() const { return submeshes; }

    Vector3 getBoundsCenter() const { return boundsCenter; }
    Vector3 getBoundsExtents() const { return boundsExtents; }

    void drawUI() override;

    void serialize(IArchive& archive) override;

protected:
    std::vector<Vertex> vertices;
    std::vector<uint8_t> indices;
    DXGI_FORMAT indexFormat = DXGI_FORMAT_R8_UINT;

    std::vector<Submesh> submeshes;

    Vector3 boundsCenter = Vector3::Zero;
    Vector3 boundsExtents = Vector3::Zero;
};