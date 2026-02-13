#pragma once
#include "Component.h"
#include "BasicMesh.h"
#include "BasicMaterial.h"
#include "AABB.h"

namespace tinygltf { class Model; }

struct ModelData {
	Matrix model;
	Matrix normalMat;
	BasicMaterial::BDRFPhongMaterialData material;
};

class BasicModel : public Component
{
public:
	BasicModel(int id, GameObject* gameObject) : Component(id, ComponentType::MODEL, gameObject) {};
	~BasicModel();
	void load(const char* fileName, const char* basePath);
	std::vector<BasicMesh*>		getMeshes() const { return m_meshes; }
	std::vector<BasicMaterial*>	getMaterials() const { return m_materials; }

#pragma region Loop functions
	bool init() override;
	void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) override;
	bool cleanUp() override;
#pragma endregion

	void drawUi() override;

	void onTransformChange() override;
private:
	std::vector<BasicMesh*>		m_meshes;
	std::vector<BasicMaterial*>	m_materials;

	std::string m_modelPath;
	std::string m_basePath;

	AABB m_aabb = {};
};
