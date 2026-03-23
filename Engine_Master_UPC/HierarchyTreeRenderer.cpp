#include "Globals.h"
#include "HierarchyTreeRenderer.h"

#include <imgui.h>

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

#include "PrefabUI.h"
#include "PrefabManager.h"
#include "PrefabEditSession.h"
#include "PrefabAsset.h"


void HierarchyTreeRenderer::renderNode(GameObject* gameObject, bool  prefabMode, SelectionState& state) const
{
    Transform* transform = gameObject->GetTransform();
    const auto children = transform->getAllChildren();

    PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
    const bool isEditRoot = prefabMode && session && gameObject == session->m_rootObject;
    const bool isPrefabInst = !isEditRoot && PrefabManager::isPrefabInstance(gameObject);

    ImGuiTreeNodeFlags flags =
        children.empty()
        ? (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen)
        : ImGuiTreeNodeFlags_OpenOnArrow;

    if (isEditRoot) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (isEditRoot) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.20f, 1.f));
    }
    else if (isPrefabInst) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.75f, 1.0f, 1.f));
    }

    const char* rawLabel = (isEditRoot && session) ? session->m_sourcePath.filename().string().c_str() : gameObject->GetName().c_str();

    std::string nodeId = std::string(rawLabel) + "###" + std::to_string(gameObject->GetID());
    bool opened = ImGui::TreeNodeEx(nodeId.c_str(), flags);

    if (isEditRoot || isPrefabInst) ImGui::PopStyleColor();

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        state.pendingSelection = gameObject;
        state.isDragging = false;
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        app->getModuleEditor()->setSelectedGameObject(gameObject);
    }

    drawContextMenu(gameObject, prefabMode, isEditRoot);

    handleDragDropSource(gameObject, isEditRoot);
    handleDragDropTarget(gameObject);

    if (state.pendingSelection == gameObject && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (!state.isDragging && onSelect)
        {
            onSelect(gameObject);
        }

        state.pendingSelection = nullptr;
    }

    if (opened && !children.empty())
    {
        for (GameObject* child : children)
        {
            renderNode(child, prefabMode, state);
        }
        ImGui::TreePop();
    }
}

void HierarchyTreeRenderer::drawContextMenu(GameObject* go, bool prefabMode, bool isEditRoot) const
{
    if (!ImGui::BeginPopupContextItem()) return;

    if (prefabMode)
    {
        PrefabUI::drawNodeContextMenu(go, prefabMode, isEditRoot);
    }
    else
    {
        // Standard scene menu
        PrefabUI::drawPrefabSubMenu(go, app->getModuleScene()->getScene());
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
        if (ImGui::MenuItem("Delete") && onDeleteRequested)
        {
            onDeleteRequested(go);
        }

        ImGui::PopStyleColor();
    }

    ImGui::EndPopup();
}


void HierarchyTreeRenderer::handleDragDropSource(GameObject* go, bool isEditRoot) const
{
    if (isEditRoot) return;

    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("GAME_OBJECT", &go, sizeof(GameObject*));
        ImGui::Text("%s", go->GetName().c_str());
        ImGui::EndDragDropSource();
    }
}


void HierarchyTreeRenderer::handleDragDropTarget(GameObject* go) const
{
    if (!ImGui::BeginDragDropTarget()) return;

    // Reparent via drag-drop of a hierarchy node
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
    {
        GameObject* dropped = *static_cast<GameObject**>(payload->Data);
        if (dropped && dropped != go && onReparent)
            onReparent(dropped, go);
    }

    // Instantiate prefab as a child of this node
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB_ASSET"))
    {
        const std::filesystem::path sourcePath(static_cast<const char*>(payload->Data));
        if (onPrefabDropOnNode)
        {
            onPrefabDropOnNode(sourcePath, go);
        }
    }

    ImGui::EndDragDropTarget();
}