#pragma once
#include "Globals.h"

#include "Transform.h"
#include "Component.h"
#include <vector>

#include "Tag.h"
#include "Layer.h"

class GameObject {
public:
	GameObject(short newUuid);
	~GameObject();

	const short GetID() { return m_uuid; }
	const std::string& GetName() { return m_name; }
	const bool GetActive() { return m_active; }
	const bool GetStatic() { return m_isStatic; }
	const Layer GetLayer() { return m_layer; }
	const Tag GetTag() { return m_tag; }
	GameObject* GetParent() { return m_parent; }
	Transform* GetTransform() { return m_transform; }
	const std::vector<GameObject*>* getChildList() { return m_transform->getAllChildren(); }

	void SetName(std::string newName) { m_name = newName; }
	void SetActive(bool newActive) { m_active = newActive; }
	void SetStatic(bool newIsStatic) { m_isStatic = newIsStatic; }
	void SetLayer(Layer newLayer) { m_layer = newLayer; }
	void SetTag(Tag newTag) { m_tag = newTag; }
	bool AddComponent(Component* newComponent);
	bool RemoveComponent(Component* componentToRemove);

	void drawUI();

private:
	short m_uuid;
	std::string m_name;
	bool m_active = true;
	bool m_isStatic = false;
	Layer m_layer = Layer::DEFAULT;
	Tag m_tag = Tag::DEFAULT;
	GameObject* m_parent = nullptr;
	Transform* m_transform;
	std::vector<Component*> m_components;
};