#pragma once
#include "EditorWindow.h"
#include "HierarchyTreeRenderer.h"

class GameObject;
class Scene;

using UID = uint64_t;

class WindowHierarchy : public EditorWindow
{
public:
    WindowHierarchy();

    void        render()        override;
    const char* getWindowName() const override { return "Hierarchy"; }

    void reparent(GameObject* child, GameObject* newParent);

    void startRename(GameObject* target);

private:
    void drawSceneHeader();
    void drawPrefabHeader(struct PrefabEditSession* session);

    void drawSceneTree();
    void drawPrefabTree(struct PrefabEditSession* session);

    void drawInlineRename();

    void onSelectCallback(GameObject* go);
    void onReparentCallback(GameObject* child, GameObject* newParent);
    void onPrefabDropOnNode(const std::filesystem::path& sourcePath, GameObject* parent);
    void onDeleteRequestedCallback(GameObject* go);

    HierarchyTreeRenderer               m_treeRenderer;
    HierarchyTreeRenderer::SelectionState m_selectionState;

    UID  m_renameTargetID = 0;
    char m_renameBuffer[256] = {};
    bool m_renameFocusPending = false;
};