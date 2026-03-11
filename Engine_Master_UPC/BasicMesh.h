#pragma once
#include "Globals.h"

#include "ICacheable.h"

class MeshAsset;

class VertexBuffer;
class IndexBuffer;

struct Submesh;

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

private:
	std::unique_ptr<VertexBuffer>	m_vertexBuffer;
	std::unique_ptr<IndexBuffer>	m_indexBuffer;

	std::vector<Submesh>			m_submeshes;

	std::vector<Vector3>	m_vertexPositions;
	std::vector<uint8_t>	m_indices;
};
