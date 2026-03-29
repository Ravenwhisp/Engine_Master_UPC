#pragma once
#include "EditorWindow.h"

class ModuleEditor;
class ModuleScene;
class ViewHierarchyDialog;
class GameObject;
class WindowHierarchy : public EditorWindow
{

public:
	WindowHierarchy();

	void		render() override;
	const char* getWindowName() const override { return "WindowHierarchy"; }
	void		addGameObject();
	void		removeGameObject();

	void startRename(GameObject* go);
	void reparent(GameObject* child, GameObject* newParent);
	GameObject* addChildToPrefabRoot(GameObject* parent);
private:
	bool createTreeNode();
	bool createTreeNode(GameObject* gameObject, bool prefabMode);


	GameObject* m_pendingSelection = nullptr;
	bool m_isDragging = false;

	ModuleEditor* m_editorModule;
	ModuleScene* m_sceneModule;

	ViewHierarchyDialog* m_viewHierarchyDialog;

#pragma region Rename Variables
	GameObject* m_renamingObject = nullptr;
	char m_renameBuffer[256];
	//	float m_lastClickTime = 0.0f;
#pragma endregion
};
