#include "Globals.h"
#include "BasicMesh.h"

#include "UtilityGLFT.h"
#include "Application.h"
#include "ResourcesModule.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"


BasicMesh::BasicMesh(const UID uid, const MeshAsset& asset) : ICacheable(uid)
{
	m_submeshes = asset.getSubmeshes();

	m_vertexPositions = asset.getVerticesPositions();
	m_indices = asset.getIndexDataVector();

	m_vertexBuffer = app->getResourcesModule()->createVertexBuffer(asset.getVertexData(), asset.getVertexCount(), asset.getVertexStride());

	if (asset.getIndexBufferSize() > 0) 
	{
		m_indexBuffer = app->getResourcesModule()->createIndexBuffer(asset.getIndexData(), asset.getIndexCount(), asset.getIndexFormat());
	}
}

bool BasicMesh::hasIndexBuffer() const
{
	return m_indexBuffer && m_indexBuffer->getD3D12Resource();
}
