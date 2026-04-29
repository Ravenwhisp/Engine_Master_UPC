#pragma once

#include "UID.h"
#include "Tag.h"
#include "Layer.h"
#include "ComponentType.h"
#include "PrefabInfo.h"

#include <vector>
#include <memory>
#include <string>

#include <rapidjson/document.h>
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp> 
#include <cereal/access.hpp>

class Component;
class ModelComponent;
class Transform;
class SceneSnapshot;

class GameObject 
{
public:
	friend class cereal::access;

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

	void SetID(UID newUid) {m_uuid = newUid;}
	void SetName(std::string newName) { m_name = newName; }
	void SetActive(bool newActive) { m_active = newActive; }
	void SetStatic(bool newIsStatic) { m_isStatic = newIsStatic; }
	void SetLayer(Layer newLayer) { m_layer = newLayer; }
	void SetTag(Tag newTag) { m_tag = newTag; }
#pragma endregion

#pragma region Prefab
	void setPrefabInfo(UID prefabId) { m_prefabInfo.m_assetUID = prefabId; }
	PrefabInfo& GetPrefabInfo() { return m_prefabInfo; }
	const PrefabInfo& GetPrefabInfo() const { return m_prefabInfo; }

	bool IsPrefabInstance() const { return m_prefabInfo.isInstance(); }
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

	template<class Archive>
	static void load_and_construct(Archive& ar, cereal::construct<GameObject>& construct)
	{
		UID uuid;
		ar(uuid);

		construct(uuid);

		ar(construct->m_name,
			construct->m_active,
			construct->m_isStatic,
			construct->m_layer,
			construct->m_tag,
			construct->m_components);
	}

	template<class Archive>
	void save(Archive& ar) const
	{
		ar(m_uuid,
			m_name,
			m_active,
			m_isStatic,
			m_layer,
			m_tag,
			m_components);
	}
#pragma endregion

#pragma region GameLoop
	bool init();
	void update();
	void lateUpdate();
	bool cleanUp();
#pragma endregion

	void drawUI();
	void moveComponent(size_t fromIndex, size_t toIndex);
	int findComponentIndex(const Component* component) const;

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
