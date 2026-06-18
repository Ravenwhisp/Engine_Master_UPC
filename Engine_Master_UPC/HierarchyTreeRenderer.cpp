#include "Globals.h"
#include "HierarchyTreeRenderer.h"

#include <imgui.h>

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "PrefabInstanceComponent.h"
#include "Transform.h"

#include "PrefabUI.h"
#include "PrefabEditSession.h"

void HierarchyTreeRenderer::renderNode(GameObject* gameObject, bool prefabMode, SelectionState& state) const
{
    Transform* transform = gameObject->GetTransform();
    const auto children = transform->getAllChildren();

    PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
    const bool isEditRoot = prefabMode && session && gameObject == session->m_rootObject;
    auto* _hierPreComp = gameObject->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    const bool isPrefabInst = !isEditRoot && _hierPreComp && _hierPreComp->isInstance();

    ImGuiTreeNodeFlags flags = children.empty()
        ? (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen)
        : ImGuiTreeNodeFlags_OpenOnArrow;

    GameObject* selected = app->getModuleEditor()->getSelectedGameObject();

    if (gameObject == selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (isEditRoot) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (isEditRoot) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.20f, 1.f));
    } else if (isPrefabInst) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.75f, 1.0f, 1.f));
    }

    const std::string label = (isEditRoot && session)
        ? session->m_sourcePath.filename().string()
        : gameObject->GetName();

    if (shouldOpenNodeToRevealGameObject(gameObject))
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    const std::string nodeId = label + "###" + std::to_string(gameObject->GetID());
    const bool opened = ImGui::TreeNodeEx(nodeId.c_str(), flags);

    if (gameObject == m_gameObjectToReveal)
    {
        ImGui::SetScrollHereY(0.5f);
        const_cast<HierarchyTreeRenderer*>(this)->clearGameObjectToReveal();
    }

    if (isEditRoot || isPrefabInst) {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state.pendingSelection = gameObject;
        state.isDragging = false;
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        app->getModuleEditor()->setSelectedGameObject(gameObject);
    }

    drawContextMenu(gameObject, prefabMode, isEditRoot);
    handleDragDropSource(gameObject, isEditRoot, state);
    handleDragDropTarget(gameObject);

    if (state.pendingSelection == gameObject && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (!state.isDragging) {
            const_cast<HierarchySelectEvent&>(OnSelect).Broadcast(gameObject);
        }
        state.pendingSelection = nullptr;
    }

    if (opened && !children.empty()) {
        for (GameObject* child : children) {
            renderNode(child, prefabMode, state);
        }
        ImGui::TreePop();
    }
}

bool HierarchyTreeRenderer::shouldOpenNodeToRevealGameObject(GameObject* currentNode) const
{
    if (!currentNode || !m_gameObjectToReveal || currentNode == m_gameObjectToReveal)
    {
        return false;
    }

    Transform* currentTransform = currentNode->GetTransform();
    Transform* targetTransform = m_gameObjectToReveal->GetTransform();

    if (!currentTransform || !targetTransform)
    {
        return false;
    }

    return targetTransform->isDescendantOf(currentTransform);
}

void HierarchyTreeRenderer::drawContextMenu(GameObject* go, bool prefabMode, bool isEditRoot) const
{
    if (!ImGui::BeginPopupContextItem()) return;

    if (prefabMode) {
        PrefabUI::drawNodeContextMenu(prefabMode);
    } else {
        PrefabUI::drawPrefabSubMenu(go, app->getModuleScene()->getScene());
        ImGui::Separator();

        const_cast<HierarchyContextMenuOpenEvent&>(OnContextMenuOpen).Broadcast(go, prefabMode, isEditRoot);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
        if (ImGui::MenuItem("Delete")) {
            const_cast<HierarchyDeleteRequestEvent&>(OnDeleteRequested).Broadcast(go);
        }
        ImGui::PopStyleColor();
    }

    ImGui::EndPopup();
}

void HierarchyTreeRenderer::handleDragDropSource(GameObject* go, bool isEditRoot, SelectionState& state) const
{
    if (isEditRoot) {
        return;
    }

    if (ImGui::BeginDragDropSource())
    {
        state.isDragging = true;
        ImGui::SetDragDropPayload("GAME_OBJECT", &go, sizeof(GameObject*));
        ImGui::Text("%s", go->GetName().c_str());
        ImGui::EndDragDropSource();
    }
}

void HierarchyTreeRenderer::handleDragDropTarget(GameObject* go) const
{
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }

    if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("GAME_OBJECT")) {
        GameObject* dropped = *static_cast<GameObject**>(p->Data);
        if (dropped && dropped != go) {
            const_cast<HierarchyReparentEvent&>(OnReparent).Broadcast(dropped, go);
        }
    }

    if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("PREFAB_ASSET")) {
        const std::filesystem::path sourcePath(static_cast<const char*>(p->Data));
        const_cast<HierarchyPrefabDropEvent&>(OnPrefabDropOnNode).Broadcast(sourcePath, go);
    }

    ImGui::EndDragDropTarget();
}
