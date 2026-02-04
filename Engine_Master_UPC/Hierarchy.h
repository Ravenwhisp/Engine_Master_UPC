#pragma once
#include "EditorWindow.h"
#include "GameObject.h"

namespace Emeika { class Scene; }

class Hierarchy: public EditorWindow
{
public:
	Hierarchy();

	void setOnSelectedGameObject(std::function<void(GameObject*)> f) { m_onSelectedGameObject.push_back(f); }
	void		render() override;
	const char* getWindowName() const override { return "Hierarchy"; }
	void		addGameObject();

private:
	void createTreeNode(Emeika::Scene* scene);
	void createTreeNode(GameObject* gameObject);
	void reparent(GameObject* child, GameObject* newParent);

	Emeika::Scene* m_scene;
	std::vector<std::function<void(GameObject*)>> m_onSelectedGameObject;
};

