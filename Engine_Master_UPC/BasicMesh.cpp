#include "Globals.h"
#include "BasicMesh.h"

#include "UtilityGLFT.h"
#include "Application.h"
#include "ResourcesModule.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"

BasicMesh::~BasicMesh()
{
	delete m_vertexBuffer;
	delete m_indexBuffer;
}

void BasicMesh::load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
	m_materialIndex = primitive.material;

	const auto& itPos = primitive.attributes.find("POSITION");
	if (itPos != primitive.attributes.end())
	{
		uint32_t numVertices = uint32_t(model.accessors[itPos->second].count);
		Vertex* vertices = new Vertex[numVertices];
		uint8_t* vertexData = (uint8_t*)vertices;
		loadAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), numVertices, model, itPos->second);
		loadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), numVertices, model, primitive.attributes, "TEXCOORD_0");
		loadAccessorData(vertexData + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), numVertices, model, primitive.attributes, "NORMAL");

		m_boundsMin = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
		m_boundsMax = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (uint32_t i = 0; i < numVertices; ++i)
		{
			m_boundsMin.x = std::min(m_boundsMin.x, vertices[i].position.x);
			m_boundsMin.y = std::min(m_boundsMin.y, vertices[i].position.y);
			m_boundsMin.z = std::min(m_boundsMin.z, vertices[i].position.z);

			m_boundsMax.x = std::max(m_boundsMax.x, vertices[i].position.x);
			m_boundsMax.y = std::max(m_boundsMax.y, vertices[i].position.y);
			m_boundsMax.z = std::max(m_boundsMax.z, vertices[i].position.z);
		}
		m_hasBounds = (numVertices > 0);

		m_vertexBuffer = app->getResourcesModule()->createVertexBuffer(vertices, numVertices, sizeof(Vertex));

		if (primitive.indices >= 0) 
		{
			const tinygltf::Accessor& indAcc = model.accessors[primitive.indices];
			if (indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT ||
				indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT ||
				indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
			{
				uint32_t indexElementSize = tinygltf::GetComponentSizeInBytes(indAcc.componentType);
				uint32_t numIndices = uint32_t(indAcc.count);
				uint8_t* indices = new uint8_t[numIndices * indexElementSize];
				loadAccessorData(indices, indexElementSize, indexElementSize, numIndices, model, primitive.indices);

				if (numIndices > 0) 
				{
					m_indexBuffer = app->getResourcesModule()->createIndexBuffer(indices, numIndices, INDEX_FORMATS[indexElementSize >> 1]);
				}
			}
		}
	}
}

bool BasicMesh::hasIndexBuffer() const
{
	return m_indexBuffer->getD3D12Resource() != nullptr;
}

void BasicMesh::draw(ID3D12GraphicsCommandList* commandList) const
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
}
