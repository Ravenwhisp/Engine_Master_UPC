#pragma once
#include "EditorWindow.h"

class EditorModule;
class SceneModule;

class ViewHierarchyDialog;

class GameObject;

class Hierarchy: public EditorWindow
{
private:
	EditorModule* m_editorModule;
	SceneModule* m_sceneModule;

	ViewHierarchyDialog* m_viewHierarchyDialog;

#pragma region Rename Variables
	GameObject* m_renamingObject = nullptr;
	char m_renameBuffer[256];
	float m_lastClickTime = 0.0f;
#pragma endregion

public:
	Hierarchy();

	void		render() override;
	const char* getWindowName() const override { return "Hierarchy"; }

	void startRename(GameObject* go);
private:
	void createTreeNode();
	void createTreeNode(GameObject* gameObject);
	void reparent(GameObject* child, GameObject* newParent);
};

