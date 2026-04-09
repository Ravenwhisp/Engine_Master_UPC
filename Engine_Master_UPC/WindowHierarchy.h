#pragma once
#include "EditorWindow.h"
#include "HierarchyTreeRenderer.h"
#include "Delegates.h"

class GameObject;
class Scene;
class ViewHierarchyDialog;

class WindowHierarchy : public EditorWindow
{
public:
    WindowHierarchy();
    ~WindowHierarchy();

    void drawInternal() override;

    const char* getWindowName() const override
    {
        return "Hierarchy";
    }

    void reparent(GameObject* child, GameObject* newParent);
    void startRename(GameObject* target);
    void addChildToPrefabRoot(GameObject* parent);

private:
    void drawSceneHeader();
    void drawPrefabHeader(struct PrefabEditSession* session);
    void drawSceneTree();
    void drawPrefabTree(struct PrefabEditSession* session);
    void drawInlineRename();
    void drawBackgroundContextMenu(bool prefabMode, struct PrefabEditSession* session);

    void onSelect(GameObject* go);
    void onReparent(GameObject* child, GameObject* newParent);
    void onPrefabDropOnNode(const std::filesystem::path& sourcePath, GameObject* parent);
    void onDeleteRequested(GameObject* go);
    void onContextMenuOpen(GameObject* go, bool prefabMode, bool isEditRoot);

    HierarchyTreeRenderer m_treeRenderer;
    HierarchyTreeRenderer::SelectionState m_selectionState;

    ViewHierarchyDialog* m_viewHierarchyDialog = nullptr;

    uint64_t m_renameTargetID = 0;
    char m_renameBuffer[256] = {};
    bool m_renameFocusPending = false;
};
