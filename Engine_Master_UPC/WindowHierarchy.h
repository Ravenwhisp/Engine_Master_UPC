#pragma once
#include "EditorWindow.h"
#include "HierarchyTreeRenderer.h"
#include "Delegates.h"

class GameObject;
class Scene;

class ModuleEditor;
class ModuleScene;
class ViewHierarchyDialog;
class GameObject;
class WindowHierarchy : public EditorWindow
{

public:

    WindowHierarchy();

    void drawInternal() override;

    const char* getWindowName() const override
    {
        return "Hierarchy";
    }

    void reparent(GameObject* child, GameObject* newParent);

    void startRename(GameObject* target);

	void startRename(GameObject* go);
	void reparent(GameObject* child, GameObject* newParent);
	GameObject* addChildToPrefabRoot(GameObject* parent);
private:

    void drawSceneHeader();

    void drawPrefabHeader(struct PrefabEditSession* session);

    void drawSceneTree();

    void drawPrefabTree(struct PrefabEditSession* session);

    void drawInlineRename();

    void onSelect(GameObject* go);

    void onReparent(GameObject* child, GameObject* newParent);

    void onPrefabDropOnNode(const std::filesystem::path& sourcePath, GameObject* parent);

    void onDeleteRequested(GameObject* go);
	bool createTreeNode();
	bool createTreeNode(GameObject* gameObject, bool prefabMode);


    HierarchyTreeRenderer m_treeRenderer;
    HierarchyTreeRenderer::SelectionState m_selectionState;
    uint64_t m_renameTargetID = 0;
    char m_renameBuffer[256] = {};
    bool m_renameFocusPending = false;
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
