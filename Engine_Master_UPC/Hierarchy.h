#pragma once
#include "EditorWindow.h"
#include "GameObject.h"


class Hierarchy: public EditorWindow
{
public:
	Hierarchy();

	void setOnSelectedGameObject(std::function<void(GameObject*)> f) { m_onSelectedGameObject.push_back(f); }
	void		render() override;
	const char* getWindowName() const override { return "Hierarchy"; }
	void		addGameObject();

private:
	void createTreeNode();
	void createTreeNode(GameObject* gameObject);
	void reparent(GameObject* child, GameObject* newParent);

	std::vector<std::function<void(GameObject*)>> m_onSelectedGameObject;
};

