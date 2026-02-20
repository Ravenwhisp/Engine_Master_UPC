#pragma once
#include "Globals.h"

#include "Transform.h"
#include "Component.h"
#include <vector>

#include "Tag.h"
#include "Layer.h"
#include "UID.h"

class BasicModel;

class GameObject {
public:
	GameObject(UID newUuid);
	~GameObject();
	
#pragma region Properties
	UID GetID() const { return m_uuid; }
	const std::string& GetName() const { return m_name; }
	bool GetActive() const { return m_active; }
	bool GetStatic() const { return m_isStatic; }
	Layer GetLayer() const { return m_layer; }
	Tag GetTag() const { return m_tag; }

	void SetName(std::string newName) { m_name = newName; }
	void SetActive(bool newActive) { m_active = newActive; }
	void SetStatic(bool newIsStatic) { m_isStatic = newIsStatic; }
	void SetLayer(Layer newLayer) { m_layer = newLayer; }
	void SetTag(Tag newTag) { m_tag = newTag; }
#pragma endregion

#pragma region Components
	Transform* GetTransform() { return m_transform; }
	const Transform* GetTransform() const { return m_transform; }
	bool AddComponent(const ComponentType componentType);
	bool RemoveComponent(Component* componentToRemove);
	Component* GetComponent(ComponentType type) const;

	template<typename T>
	T* GetComponentAs(ComponentType type) const
	{
		return static_cast<T*>(GetComponent(type));
	}
#pragma endregion

#pragma endregion


#pragma region GameLoop
	bool init();
	void update();
	void preRender();
	void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix);
	void postRender();
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

	Transform* m_transform;
	std::vector<Component*> m_components;

	//Testing duck
	BasicModel* m_model;
	//////////////
};
