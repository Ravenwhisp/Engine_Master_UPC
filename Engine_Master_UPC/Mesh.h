#pragma once
#include "Globals.h"

class VertexBuffer;
class IndexBuffer;

namespace tinygltf { class Model;  struct Mesh; struct Primitive; }
namespace Emeika {

	class Mesh {
	public:
		~Mesh();
		void load(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const tinygltf::Primitive& primitive);
		IndexBuffer*	getVertexBuffer() { return m_indexBuffer; }
		VertexBuffer*	getIndexBuffer() { return m_vertexBuffer; }

		bool hasIndexBuffer() const;

		int32_t getMaterialIndex() const { return m_materialIndex; }

		void draw(ID3D12GraphicsCommandList* commandList) const;

	private:
		VertexBuffer*	m_vertexBuffer;
		IndexBuffer*	m_indexBuffer;

		int32_t m_materialIndex{ -1 };
	};
}

