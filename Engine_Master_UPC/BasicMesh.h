#pragma once
#include "Globals.h"
#include "Asset.h"

class VertexBuffer;
class IndexBuffer;

namespace tinygltf { class Model;  struct Mesh; struct Primitive; }

class BasicMesh
{
public:
	~BasicMesh();
	void load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);
	IndexBuffer* getVertexBuffer() { return m_indexBuffer; }
	VertexBuffer* getIndexBuffer() { return m_vertexBuffer; }

	bool hasIndexBuffer() const;

	int32_t getMaterialIndex() const { return m_materialIndex; }

	void draw(ID3D12GraphicsCommandList* commandList) const;

	bool hasBounds() const { return m_hasBounds; }
	const Vector3& getBoundsMin() const { return m_boundsMin; }
	const Vector3& getBoundsMax() const { return m_boundsMax; }

	const std::vector<Vector3>& getPositionsCPU() const { return m_positionsCPU; }
	const std::vector<uint32_t>& getIndicesCPU() const { return m_indicesCPU; }

private:
	VertexBuffer* m_vertexBuffer;
	IndexBuffer* m_indexBuffer;

	int32_t m_materialIndex{ -1 };

	bool    m_hasBounds = false;
	Vector3 m_boundsMin = Vector3(0, 0, 0);
	Vector3 m_boundsMax = Vector3(0, 0, 0);

	std::vector<Vector3> m_positionsCPU;
	std::vector<uint32_t> m_indicesCPU;
};
