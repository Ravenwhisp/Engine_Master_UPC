#pragma once
#include "EditorWindow.h"
#include "GameObject.h"
#include <filesystem>


class WindowHierarchy : public EditorWindow
{
public:
	WindowHierarchy();

	void		render() override;
	const char* getWindowName() const override { return "WindowHierarchy"; }
	void		addGameObject();
	void		removeGameObject();

private:
	void createTreeNode();
	void createTreeNode(GameObject* gameObject);
	void reparent(GameObject* child, GameObject* newParent);

	GameObject* m_pendingSelection = nullptr;
	bool m_isDragging = false;
};