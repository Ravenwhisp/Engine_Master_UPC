#pragma once
#include "EditorWindow.h"
#include "GameObject.h"


class Hierarchy: public EditorWindow
{
public:
	Hierarchy();

	void		render() override;
	const char* getWindowName() const override { return "Hierarchy"; }
	void		addGameObject();
	void		removeGameObject();

private:
	void createTreeNode();
	void createTreeNode(GameObject* gameObject);
	void reparent(GameObject* child, GameObject* newParent);
};

