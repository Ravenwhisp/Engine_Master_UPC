#pragma once

#include "UID.h"
#include "Tag.h"
#include "Layer.h"
#include "ComponentType.h"
#include "PrefabAsset.h"

#include <vector>
#include <memory>
#include <string>

#include <rapidjson/document.h>

class Component;
class ModelComponent;
class Transform;
class SceneSnapshot;

struct PrefabInfo
{
	UID m_prefabUID = 0; 
	MD5Hash m_assetUID;      
	std::filesystem::path m_sourcePath;
	std::string m_name;           

	PrefabOverrideRecord m_overrides;    

	bool m_isPrefabRoot = false;

	bool isInstance() const { return m_prefabUID != 0 && !m_sourcePath.empty(); }

	void clear()
	{
		m_prefabUID = 0;
		m_assetUID = {};
		m_sourcePath.clear();
		m_name.clear();
		m_overrides.clear();
		m_isPrefabRoot = false;
	}
};
class GameObject 
{
public:
	GameObject(UID newUuid);
	GameObject(UID newUuid, UID transformUuid);
	~GameObject();
	std::unique_ptr<GameObject> clone() const;
	
#pragma region Properties
	UID GetID() const { return m_uuid; }
	const std::string& GetName() const { return m_name; }
	bool GetActive() const { return m_active; }
	bool IsActiveInWindowHierarchy() const;
	bool GetStatic() const { return m_isStatic; }
	Layer GetLayer() const { return m_layer; }
	Tag GetTag() const { return m_tag; }

	void SetName(std::string newName) { m_name = newName; }
	void SetActive(bool newActive) { m_active = newActive; }
	void SetStatic(bool newIsStatic) { m_isStatic = newIsStatic; }
	void SetLayer(Layer newLayer) { m_layer = newLayer; }
	void SetTag(Tag newTag) { m_tag = newTag; }
#pragma endregion


#pragma region Prefab
	PrefabInfo& GetPrefabInfo() { return m_prefabInfo; }
	const PrefabInfo& GetPrefabInfo() const { return m_prefabInfo; }

	bool IsPrefabInstance() const { return m_prefabInfo.isInstance(); }
	bool IsPrefabRoot()     const { return m_prefabInfo.m_isPrefabRoot; }
#pragma endregion

#pragma region Components
	Transform* GetTransform() { return m_transform; }
	const Transform* GetTransform() const { return m_transform; }
	bool AddComponent(const ComponentType componentType);
	Component* AddComponentWithUID(const ComponentType componentType, UID id);
	bool AddClonedComponent(std::unique_ptr<Component> component);
	bool RemoveComponent(Component* componentToRemove);
	Component* GetComponent(ComponentType type) const;
	std::vector<Component*> GetAllComponents() const;

	template<typename T>
	T* GetComponentAs(ComponentType type) const
	{
		return static_cast<T*>(GetComponent(type));
	}
#pragma endregion

#pragma region Persistence
	rapidjson::Value getJSON(rapidjson::Document& domTree);
	bool deserializeJSON(const rapidjson::Value& gameObjectJson, uint64_t& outParentUid);
#pragma endregion

#pragma region GameLoop
	bool init();
	void update();
	bool cleanUp();
#pragma endregion

	void drawUI();

	void onTransformChange();

private:
	UID m_uuid;

	std::string m_name;
	bool m_active = true;
	bool m_isStatic = false;
	Layer m_layer = Layer::DEFAULT;
	Tag m_tag = Tag::DEFAULT;

	PrefabInfo m_prefabInfo;

	std::vector<std::unique_ptr<Component>> m_components;
	Transform* m_transform;
};
