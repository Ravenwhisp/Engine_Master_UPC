#include "Globals.h"
#include "BasicMesh.h"

#include "Application.h"
#include "ModuleResources.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "ModelAsset.h"


BasicMesh::BasicMesh(const MD5Hash uid, const MeshAsset& asset) : ICacheable(uid)
{
	m_submeshes = asset.getSubmeshes();

	m_vertexPositions = asset.getVerticesPositions();
	m_indices = asset.getIndexDataVector();

	m_vertexBuffer.reset(app->getModuleResources()->createVertexBuffer(asset.getVertexData(), asset.getVertexCount(), asset.getVertexStride()));

	if (asset.getIndexBufferSize() > 0) 
	{
		m_indexBuffer.reset(app->getModuleResources()->createIndexBuffer(asset.getIndexData(), asset.getIndexCount(), asset.getIndexFormat()));
	}
}

BasicMesh::~BasicMesh() = default;

bool BasicMesh::hasIndexBuffer() const
{
	return m_indexBuffer && m_indexBuffer->getD3D12Resource();
}
