#pragma once
#include "EditorWindow.h"

class EditorModule;
class SceneModule;
class ViewHierarchyDialog;
class GameObject;
class WindowHierarchy: public EditorWindow
{
private:
	EditorModule* m_editorModule;
	SceneModule* m_sceneModule;

	ViewHierarchyDialog* m_viewHierarchyDialog;

	GameObject* m_pendingSelection = nullptr;
	bool m_isDragging = false;

#pragma region Rename Variables
	GameObject* m_renamingObject = nullptr;
	char m_renameBuffer[256];
	float m_lastClickTime = 0.0f;
#pragma endregion

public:
	WindowHierarchy();

	void		render() override;
	const char* getWindowName() const override { return "WindowHierarchy"; }
	void		addGameObject();
	void		removeGameObject();

	void startRename(GameObject* go);
	void reparent(GameObject* child, GameObject* newParent);
private:
	void createTreeNode();
	void createTreeNode(GameObject* gameObject);
};

