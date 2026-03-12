#include "Globals.h"
#include "BasicMesh.h"

#include "Application.h"
#include "ModuleResources.h"

#include "ModelAsset.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

BasicMesh::BasicMesh(const UID uid, const MeshAsset& asset) : ICacheable(uid)
{
	m_submeshes = asset.getSubmeshes();

	m_vertexPositions = asset.getVerticesPositions();
	m_indices = asset.getIndexDataVector();

	m_vertexBuffer = app->getModuleResources()->createVertexBuffer(asset.getVertexData(), asset.getVertexCount(), asset.getVertexStride());

	if (asset.getIndexBufferSize() > 0) 
	{
		m_indexBuffer = app->getModuleResources()->createIndexBuffer(asset.getIndexData(), asset.getIndexCount(), asset.getIndexFormat());
	}
}

BasicMesh::~BasicMesh() = default;

bool BasicMesh::hasIndexBuffer() const
{
	return m_indexBuffer && m_indexBuffer->getD3D12Resource();
}
