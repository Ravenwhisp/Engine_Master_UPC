#pragma once
#include "Globals.h"

class VertexBuffer;
class IndexBuffer;

namespace tinygltf { class Model;  struct Mesh; struct Primitive; }
namespace Emeika {

	class Mesh {
	public:
		~Mesh();
		void Load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);
		IndexBuffer* GetVertexBuffer() { return indexBuffer; }
		VertexBuffer* GetIndexBuffer() { return vertexBuffer; }

		bool HasIndexBuffer() const;

		int32_t GetMaterialIndex() const { return _materialIndex; }

		void Draw(ID3D12GraphicsCommandList* commandList) const;

	private:
		VertexBuffer* vertexBuffer;
		IndexBuffer* indexBuffer;

		int32_t _materialIndex{ -1 };
	};
}

