#pragma once
#include "Globals.h"
#include "Asset.h"
#include "Vertex.h"
#include <IndexBuffer.h>


struct Submesh
{
	uint32_t indexStart;
	uint32_t indexCount;
	MD5Hash materialId;
};


class MeshAsset : public Asset
{
public:
	friend class ImporterMesh;
	friend class ImporterGltf;

	MeshAsset() {}
	MeshAsset(MD5Hash id) : Asset(id, AssetType::MESH) {}

	const void* getVertexData() const { return vertices.data(); }
	std::vector<Vector3> getVerticesPositions() const {
		std::vector<Vector3> positions;
		for (const auto& vertex : vertices) {
			positions.push_back(vertex.position);
		}
		return positions;
	}

	uint32_t	getVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
	uint32_t	getVertexStride() const { return sizeof(Vertex); }

	const void* getIndexData() const { return indices.data(); }
	std::vector<uint8_t> getIndexDataVector() const { return indices; }
	uint32_t	getIndexBufferSize() const { return static_cast<uint32_t>(indices.size()); }
	uint32_t getIndexCount() const {
		return static_cast<uint32_t>(indices.size()) / getSizeByFormat(indexFormat);
	}
	DXGI_FORMAT getIndexFormat() const { return indexFormat; }

	const std::vector<Submesh>& getSubmeshes() const { return submeshes; }

	Vector3 getBoundsCenter() const { return boundsCenter; }
	Vector3 getBoundsExtents() const { return boundsExtents; }
protected:
	std::vector<Vertex>		vertices;
	std::vector<uint8_t>	indices;
	DXGI_FORMAT				indexFormat = DXGI_FORMAT_R8_UINT;
	// The submeshes are tinygltf primitives, which are the smallest renderable units.
	std::vector<Submesh>	submeshes;

	Vector3 boundsCenter;
	Vector3 boundsExtents;
};
