#pragma once

#include <memory>
#include <vector>

#include "ICacheable.h"
#include "SimpleMath.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

class MeshAsset;

struct Submesh;

using DirectX::SimpleMath::Vector3;

class BasicMesh : public ICacheable
{
public:
	explicit BasicMesh(const MD5Hash uid, const MeshAsset& asset);
	~BasicMesh();

	std::unique_ptr<VertexBuffer>& getVertexBuffer() { return m_vertexBuffer; }
	std::unique_ptr<IndexBuffer>& getIndexBuffer() { return m_indexBuffer; }
	std::vector<Submesh>& getSubmeshes() { return m_submeshes; }

	std::vector<Vector3>& getVertexPositions() { return m_vertexPositions; }
	std::vector<uint8_t>& getIndices() { return m_indices; }

	bool hasIndexBuffer() const;

private:
	std::unique_ptr<VertexBuffer>	m_vertexBuffer;
	std::unique_ptr<IndexBuffer>	m_indexBuffer;

	std::vector<Submesh>			m_submeshes;

	std::vector<Vector3>	m_vertexPositions;
	std::vector<uint8_t>	m_indices;
};
