#include "Globals.h"
#include "BasicMesh.h"

#include "UtilityGLFT.h"
#include "Application.h"
#include "ResourcesModule.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"


BasicMesh::BasicMesh(const MeshAsset& asset)
{
	m_submeshes = asset.getSubmeshes();

	m_vertexBuffer = app->getResourcesModule()->createVertexBuffer(asset.getVertexData(), asset.getVertexCount(), asset.getVertexStride());

	if (asset.getIndexBufferSize() > 0) 
	{
		m_indexBuffer = app->getResourcesModule()->createIndexBuffer(asset.getIndexData(), asset.getIndexBufferSize(), asset.getIndexFormat());

	}
}

bool BasicMesh::hasIndexBuffer() const
{
	return m_indexBuffer && m_indexBuffer->getD3D12Resource();
}

/*void BasicMesh::draw(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	D3D12_VERTEX_BUFFER_VIEW vbv = m_vertexBuffer->getVertexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);

	if (hasIndexBuffer()) 
	{
		D3D12_INDEX_BUFFER_VIEW ibv = m_indexBuffer->getIndexBufferView();
		commandList->IASetIndexBuffer(&ibv);
		commandList->DrawIndexedInstanced(m_indexBuffer->getNumIndices(), 1, 0, 0, 0);
	}
	else 
	{
		commandList->DrawInstanced(m_vertexBuffer->getNumVertices(), 1, 0, 0);
	}
}*/

void BasicMesh::draw(ID3D12GraphicsCommandList* commandList, const Submesh& submesh) const
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_VERTEX_BUFFER_VIEW vbv = m_vertexBuffer->getVertexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);

	if (hasIndexBuffer())
	{
		D3D12_INDEX_BUFFER_VIEW ibv = m_indexBuffer->getIndexBufferView();
		commandList->IASetIndexBuffer(&ibv);

		commandList->DrawIndexedInstanced(submesh.indexCount, 1, submesh.indexStart, 0, 0 );
	}
}
