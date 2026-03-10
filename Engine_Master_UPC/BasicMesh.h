#pragma once
#include "Globals.h"
#include "Asset.h"
#include "ModelAsset.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ICacheable.h"

namespace tinygltf { class Model;  struct Mesh; struct Primitive; }

class BasicMesh : public ICacheable
{
public:
	explicit BasicMesh(const UID uid, const MeshAsset& asset);
	~BasicMesh() = default;
	std::unique_ptr<VertexBuffer>& getVertexBuffer() { return m_vertexBuffer; }
	std::unique_ptr<IndexBuffer>& getIndexBuffer() { return m_indexBuffer; }
	std::vector<Submesh>& getSubmeshes() { return m_submeshes; }

	std::vector<Vector3>& getVertexPositions() { return m_vertexPositions; }
	std::vector<uint8_t>& getIndices() { return m_indices; }

	bool			hasIndexBuffer() const;

	void draw(ID3D12GraphicsCommandList* commandList, const Submesh& submesh) const;
private:
	std::unique_ptr<VertexBuffer>	m_vertexBuffer;
	std::unique_ptr<IndexBuffer>	m_indexBuffer;

	std::vector<Submesh>			m_submeshes;

	std::vector<Vector3>	m_vertexPositions;
	std::vector<uint8_t>	m_indices;
};
