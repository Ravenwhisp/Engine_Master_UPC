#pragma once
#include "Component.h"
#include "BasicMesh.h"
#include "BasicMaterial.h"
#include "BoundingBox.h"
#include <ModelAsset.h>

namespace tinygltf { class Model; }

struct ModelData {
	Matrix model;
	Matrix normalMat;
	BasicMaterial::BDRFPhongMaterialData material;
};


class MeshRenderer : public Component
{
public:
	MeshRenderer(UID id, GameObject* gameObject) : Component(id, ComponentType::MODEL, gameObject) {};
	~MeshRenderer() = default;

	void addModel(ModelAsset& model);

	std::vector<std::unique_ptr<BasicMesh>>&		getMeshes() const { return m_meshes; }
	std::vector<std::unique_ptr<BasicMaterial>>&	getMaterials() const { return m_materials; }
	Engine::BoundingBox&							getBoundingBox() { return m_boundingBox; }

#pragma region Loop functions
	bool init() override;
	void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) override;
	bool cleanUp() override;
#pragma endregion

	void drawUi() override;

	void onTransformChange() override;
private:
	mutable std::vector<std::unique_ptr<BasicMesh>>		m_meshes;
	mutable std::vector<std::unique_ptr<BasicMaterial>>	m_materials;
	Engine::BoundingBox									m_boundingBox;

	std::string m_modelPath;
	std::string m_basePath;

	bool m_drawBounds = false;
	bool m_boundsDepthTest = true;
	bool m_drawWorldAabb = false;

	bool    m_hasBounds = false;
};
