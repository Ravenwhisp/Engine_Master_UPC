#pragma once
#include "Id.h"
#include <string>

class GameObjectManager;
class Component;
class Scene;

// For now we have the Layer and Tag in enums, but in the future they could be more complex structures
enum Layer {
	Default,
	TransparentFX,
	IgnoreRaycast,
	Water,
	UI,
	COUNT_LAYERS
};

enum Tag {
	Untagged,
	Player,
	Enemy,
	Collectible,
	COUNT_TAGS
};


// Following GameObject Unity API: https://docs.unity3d.com/6000.3/Documentation/ScriptReference/GameObject.html
class GameObject {
public:
	friend class GameCoreModule;

	GameObject();
	~GameObject() = default;
	GameObject(const std::string& name);

	template<class... Component>
	GameObject(const std::string& name, Component*... initialComponents);

	template<class Component>
	Component* AddComponent();

	template<class Component>
	Component* AddComponent(Component* component);

	// For now we iterate throw all the vector, which is not optimal
	template<class Component>
	Component* GetComponent();

	std::vector<Component*> GetComponents() { return _components; }


	bool AddChild(GameObject* child);
	bool RemoveChild(GameObject* child);
	std::vector<GameObject*> GetChildren() { return children; }
	bool IsChild(GameObject* potentialChild);


	bool IsChildOf(GameObject* parent);
	void SetParent(GameObject* newParent) { parent = newParent; }
	GameObject* GetParent() { return parent; }

	ID_TYPE GetId() const { return _id; }
	const char* GetName() { return (char*)_name.c_str(); }
	void SetName(const char* name) { _name = name; }
protected:
private:
	Layer layer = Layer::Default;
	Tag tag = Tag::Untagged;

	Scene* scene = nullptr;
	GameObject* parent = nullptr;
	std::vector<GameObject*> children;
	std::vector<Component*> _components;

	ID_TYPE _id;
	std::string _name;
	bool _activeInHierarchy = true;
	bool _activeSelf = true;
	bool _isStatic = false;
};

template<class ...Component>
GameObject::GameObject(const std::string& name, Component * ...initialComponents)
{
	GameObjectManager::Instance()->CreateGameObject(*this);

	(AddComponent<Component>(), ...);
}

template<class Component>
Component* GameObject::AddComponent()
{
	Component* component = new Component();
	_components.emplace_back(component);
	return component;
}

template<class Component>
Component* GameObject::AddComponent(Component* component)
{
	_components.emplace_back(component);
	return component;
}

template<class Component>
Component* GameObject::GetComponent()
{
	for (auto& component : _components)
	{
		Component* casted = dynamic_cast<Component*>(component);
		if (casted != nullptr)
		{
			return casted;
		}
	}

	return nullptr;
}


