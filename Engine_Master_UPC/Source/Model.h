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
	//TODO: right now Model class acts as a MeshRenderer component
	class Model: public Component
	{
	public:
		~Model();
		void Load(const char* fileName, const char* basePath);
		std::vector<Emeika::Mesh*> GetMeshes() { return _meshes; }
		std::vector<Emeika::Material*> GetMaterials() { return _materials; }
	private:
		std::vector<Emeika::Mesh*> _meshes;
		std::vector<Emeika::Material*> _materials;
	};
}




