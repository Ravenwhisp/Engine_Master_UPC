#pragma once
#include "Component.h"
#include "BasicMesh.h"
#include "BasicMaterial.h"
#include "BoundingBox.h"
#include "Asset.h"

namespace tinygltf { class Model; }

struct ModelData {
	Matrix model;
	Matrix normalMat;
	BasicMaterial::BDRFPhongMaterialData material;
};

class BasicModel : public Component
{
public:
	BasicModel(UID id, GameObject* gameObject) : Component(id, ComponentType::MODEL, gameObject) {}
	~BasicModel();
	void load(const char* fileName, const char* basePath);
	std::vector<BasicMesh*>		getMeshes() const { return m_meshes; }
	std::vector<BasicMaterial*>	getMaterials() const { return m_materials; }
	Engine::BoundingBox&		getBoundingBox() { return m_boundingBox; }

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
	std::vector<BasicMesh*>		m_meshes;
	std::vector<BasicMaterial*>	m_materials;

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
