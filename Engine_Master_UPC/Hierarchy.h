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
	void createTreeNode(GameObject* gameObject, bool prefabMode);
	void reparent(GameObject* child, GameObject* newParent);
	void addChildToPrefabRoot(GameObject* parent);

	GameObject* m_pendingSelection = nullptr;
	bool m_isDragging = false;
};

