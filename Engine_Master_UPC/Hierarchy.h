#pragma once
#include "EditorWindow.h"
#include "GameObject.h"

class ViewHierarchyDialog;

class Hierarchy: public EditorWindow
{
private:
	ViewHierarchyDialog* m_viewHierarchyDialog;

public:
	Hierarchy();

	void		render() override;
	const char* getWindowName() const override { return "Hierarchy"; }

private:
	void createTreeNode();
	void createTreeNode(GameObject* gameObject);
	void reparent(GameObject* child, GameObject* newParent);
};

