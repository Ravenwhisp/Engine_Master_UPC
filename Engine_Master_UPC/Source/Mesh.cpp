#include "Globals.h"
#include "UtilityGLFT.h"
#include "Application.h"
#include "ResourcesModule.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "Mesh.h"

Emeika::Mesh::~Mesh()
{
	delete vertexBuffer;
	delete indexBuffer;
}

void Emeika::Mesh::Load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive)
{
	_materialIndex = primitive.material;

	const auto& itPos = primitive.attributes.find("POSITION");
	if (itPos != primitive.attributes.end()) // If no position no geometry data
	{
		uint32_t numVertices = uint32_t(model.accessors[itPos->second].count);
		Vertex* vertices = new Vertex[numVertices];
		uint8_t* vertexData = (uint8_t*)vertices; // Casts Vertex Buffer to Bytes (uint8_t*) buffer
		LoadAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), numVertices, model, itPos->second);
		LoadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), numVertices, model, primitive.attributes, "TEXCOORD_0");
		LoadAccessorData(vertexData + offsetof(Vertex, normal), sizeof(Vector3), sizeof(Vertex), numVertices, model, primitive.attributes, "NORMAL");

		vertexBuffer = app->GetResourcesModule()->CreateVertexBuffer(vertices, numVertices, sizeof(Vertex));

		if (primitive.indices >= 0) {
			const tinygltf::Accessor& indAcc = model.accessors[primitive.indices];
			if (indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT ||
				indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT ||
				indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
			{
				uint32_t indexElementSize = tinygltf::GetComponentSizeInBytes(indAcc.componentType);
				uint32_t numIndices = uint32_t(indAcc.count);
				uint8_t* indices = new uint8_t[numIndices * indexElementSize];
				LoadAccessorData(indices, indexElementSize, indexElementSize, numIndices, model, primitive.indices);

				if (numIndices > 0) {
					indexBuffer = app->GetResourcesModule()->CreateIndexBuffer(indices, numIndices, IndexFormats[indexElementSize >> 1]);
				}
			}
		}
	}
}

bool Emeika::Mesh::HasIndexBuffer() const
{
	return indexBuffer->GetD3D12Resource() != nullptr;
}

void Emeika::Mesh::Draw(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	D3D12_VERTEX_BUFFER_VIEW vbv = vertexBuffer->GetVertexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);

	if (HasIndexBuffer()) {
		D3D12_INDEX_BUFFER_VIEW ibv = indexBuffer->GetIndexBufferView();
		commandList->IASetIndexBuffer(&ibv);
		commandList->DrawIndexedInstanced(indexBuffer->GetNumIndices(), 1, 0, 0, 0);
	}
	else {
		commandList->DrawInstanced(vertexBuffer->GetNumVertices(), 1, 0, 0);
	}
}
