#pragma once
#include "Globals.h"

class VertexBuffer;
class IndexBuffer;

namespace tinygltf { class Model;  struct Mesh; struct Primitive; }

class BasicMesh
{
public:
	~BasicMesh();
	void load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive, Vector3& minVector, Vector3& maxVector);
	IndexBuffer* getVertexBuffer() { return m_indexBuffer; }
	VertexBuffer* getIndexBuffer() { return m_vertexBuffer; }

	bool hasIndexBuffer() const;

	int32_t getMaterialIndex() const { return m_materialIndex; }

	void draw(ID3D12GraphicsCommandList* commandList) const;

private:
	VertexBuffer* m_vertexBuffer;
	IndexBuffer* m_indexBuffer;

	int32_t m_materialIndex{ -1 };
};
