#pragma once
#include <functional>
#include <filesystem>

class GameObject;
class Scene;

class HierarchyTreeRenderer
{
public:

    std::function<void(GameObject*)>                                    onSelect;
    std::function<void(GameObject*, GameObject*)>                       onReparent;
    std::function<void(const std::filesystem::path&, GameObject*)>      onPrefabDropOnNode;
    std::function<void(GameObject*)>                                    onDeleteRequested;

    struct SelectionState
    {
        GameObject* pendingSelection = nullptr;
        bool        isDragging = false;
    };

    void renderNode(GameObject* gameObject, bool prefabMode, SelectionState& state) const;

private:
    void drawContextMenu(GameObject* go, bool prefabMode, bool isEditRoot) const;
    void handleDragDropSource(GameObject* go, bool isEditRoot) const;
    void handleDragDropTarget(GameObject* go) const;
};