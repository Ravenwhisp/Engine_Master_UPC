#pragma once
#include "Globals.h"
#include "Asset.h"

#include <vector> 

class VertexBuffer;
class IndexBuffer;

namespace tinygltf { class Model;  struct Mesh; struct Primitive; }

class BasicMesh
{
public:
	~BasicMesh();
	void load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);
	IndexBuffer* getIndexBuffer() { return m_indexBuffer; }
	VertexBuffer* getVertexBuffer() { return m_vertexBuffer; }

	bool hasIndexBuffer() const;

	int32_t getMaterialIndex() const { return m_materialIndex; }

	void draw(ID3D12GraphicsCommandList* commandList) const;

	bool hasBounds() const { return m_hasBounds; }
	const Vector3& getBoundsMin() const { return m_boundsMin; }
	const Vector3& getBoundsMax() const { return m_boundsMax; }

	const std::vector<Vector3>& getCpuPositions() const { return m_cpuPositions; }
    const std::vector<uint32_t>& getCpuIndices() const { return m_cpuIndices; }
    bool hasCpuGeometry() const { return !m_cpuPositions.empty() && (!m_cpuIndices.empty()); }
   

private:
	VertexBuffer* m_vertexBuffer;
	IndexBuffer* m_indexBuffer;

	int32_t m_materialIndex{ -1 };

	bool    m_hasBounds = false;
	Vector3 m_boundsMin = Vector3(0, 0, 0);
	Vector3 m_boundsMax = Vector3(0, 0, 0);

	std::vector<Vector3>  m_cpuPositions;
	std::vector<uint32_t> m_cpuIndices;
};
