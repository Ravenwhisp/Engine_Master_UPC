#pragma once
#include "Component.h"
#include "ResourcesModule.h"
#include "BoundingBox.h"
#include "Asset.h"

namespace tinygltf { class Model; }

struct ModelData {
	Matrix model;
	Matrix normalMat;
	BasicMaterial::BDRFPhongMaterialData material;
};

class ModelComponent : public Component
{
public:
	ModelComponent(UID id, GameObject* gameObject) : Component(id, ComponentType::MODEL, gameObject) {};
	void load(const char* fileName, const char* basePath);

	const std::vector<std::unique_ptr<BasicMesh>>& getMeshes() const
	{
		static const std::vector<std::unique_ptr<BasicMesh>> empty;
		return m_modelBinaryData ? m_modelBinaryData->m_meshes : empty; // m_modelBinaryData can be null/empty if no model has been loaded yet
	}

	void clearMeshes()
	{
		if (!getMeshes().empty())
		{
			m_modelBinaryData->m_meshes.clear();
		}
	}

	const std::vector<std::unique_ptr<BasicMaterial>>& getMaterials() const
	{
		static const std::vector<std::unique_ptr<BasicMaterial>> empty;
		return m_modelBinaryData ? m_modelBinaryData->m_materials : empty; // m_modelBinaryData can be null/empty if no model has been loaded yet
	}

	void clearMaterials()
	{
		if (!getMaterials().empty())
		{
			m_modelBinaryData->m_materials.clear();
		}
	}

	Engine::BoundingBox& getBoundingBox() { return m_boundingBox; }

#pragma region Loop functions
	bool init() override;
	void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) override;
	bool cleanUp() override;
#pragma endregion

	void drawUi() override;

	void onTransformChange() override;

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
	std::shared_ptr<ModelBinaryData> m_modelBinaryData;

	std::string m_modelPath;
	std::string m_basePath;

	bool m_drawBounds = false;
	bool m_boundsDepthTest = true;
	bool m_drawWorldAabb = false;

	bool    m_hasBounds = false;
	//Vector3 m_boundsMin = Vector3(0, 0, 0);
	//Vector3 m_boundsMax = Vector3(0, 0, 0);

	Engine::BoundingBox m_boundingBox;
};