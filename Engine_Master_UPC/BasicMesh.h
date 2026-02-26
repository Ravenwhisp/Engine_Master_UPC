#pragma once
#include "Globals.h"
#include "Asset.h"
#include "ModelAsset.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

namespace tinygltf { class Model;  struct Mesh; struct Primitive; }

class BasicMesh
{
public:
	explicit BasicMesh(const MeshAsset& asset);
	~BasicMesh() = default;
	std::unique_ptr<VertexBuffer>&	getVertexBuffer() { return m_vertexBuffer; }
	std::unique_ptr<IndexBuffer>&	getIndexBuffer() { return m_indexBuffer; }
	std::vector<Submesh>&			getSubmeshes() { return m_submeshes; }

	bool			hasIndexBuffer() const;

	void draw(ID3D12GraphicsCommandList* commandList, const Submesh& submesh) const;
private:
	std::unique_ptr<VertexBuffer>	m_vertexBuffer;
	std::unique_ptr<IndexBuffer>	m_indexBuffer;

	std::vector<Submesh>			m_submeshes;
};
