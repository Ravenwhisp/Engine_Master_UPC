#pragma once
#include "Delegates.h"
#include <functional>
#include <filesystem>

class GameObject;
class Scene;

DECLARE_EVENT(HierarchySelectEvent, HierarchyTreeRenderer, GameObject*);
DECLARE_EVENT(HierarchyReparentEvent, HierarchyTreeRenderer, GameObject*, GameObject*);
DECLARE_EVENT(HierarchyPrefabDropEvent, HierarchyTreeRenderer, const std::filesystem::path&, GameObject*);
DECLARE_EVENT(HierarchyDeleteRequestEvent, HierarchyTreeRenderer, GameObject*);

class HierarchyTreeRenderer
{
public:

    HierarchySelectEvent        OnSelect;
    HierarchyReparentEvent      OnReparent;
    HierarchyPrefabDropEvent    OnPrefabDropOnNode;
    HierarchyDeleteRequestEvent OnDeleteRequested;

    struct SelectionState
    {
        GameObject* pendingSelection = nullptr;
        bool        isDragging = false;
    };

    void renderNode(GameObject* gameObject, bool prefabMode, SelectionState& state) const;

private:
    void drawContextMenu(GameObject* go, bool prefabMode, bool isEditRoot) const;
    void handleDragDropSource(GameObject* go, bool isEditRoot, SelectionState& state) const;
    void handleDragDropTarget(GameObject* go) const;
};