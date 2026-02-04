#pragma once
#include "Component.h"
#include "Mesh.h"
#include "Material.h"

namespace tinygltf { class Model; }

struct ModelData {
	Matrix model;
	Matrix normalMat;
	Emeika::Material::BDRFPhongMaterialData material;
};

namespace Emeika {
	class Model: public Component
	{
	public:
		~Model();
		void load(const char* fileName, const char* basePath);
		std::vector<Emeika::Mesh*>		getMeshes() const { return m_meshes; }
		std::vector<Emeika::Material*>	getMaterials() const { return m_materials; }
	private:
		std::vector<Emeika::Mesh*>		m_meshes;
		std::vector<Emeika::Material*>	m_materials;
	};
}




