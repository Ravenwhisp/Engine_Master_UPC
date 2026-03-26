#include "Globals.h"
#include "WindowHierarchy.h"

#include <imgui.h>

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"

#include "PrefabUI.h"
#include "PrefabEditSession.h"
#include <CommandAddGameObject.h>
#include <CommandRemoveGameObject.h>
#include <CommandRenameGameObject.h>
#include <HierarchyUtils.h>
#include <CommandReparent.h>
#include <CommandAddChildToPrefabRoot.h>
#include <CommandInstantiatePrefab.h>

#include "UID.h"

WindowHierarchy::WindowHierarchy()
{
    m_treeRenderer.OnSelect.AddRaw(this, &WindowHierarchy::onSelect);
    m_treeRenderer.OnReparent.AddRaw(this, &WindowHierarchy::onReparent);
    m_treeRenderer.OnPrefabDropOnNode.AddRaw(this, &WindowHierarchy::onPrefabDropOnNode);
    m_treeRenderer.OnDeleteRequested.AddRaw(this, &WindowHierarchy::onDeleteRequested);
}

void WindowHierarchy::drawInternal()
{
    PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
    const bool prefabMode = session && session->m_active;

    if (prefabMode)
    {
        drawPrefabHeader(session);
    }
    else
    {
        drawSceneHeader();
    }

    ImGui::Separator();

    if (prefabMode)
    {
        drawPrefabTree(session);
    }
    else
    {
        drawSceneTree();
    }

    drawInlineRename();
}

void WindowHierarchy::drawSceneHeader()
{
    Scene* scene = app->getModuleScene()->getScene();

    if (ImGui::Button("New Object"))
    {
        CommandAddGameObject(scene).run();
    }

    ImGui::SameLine();

    GameObject* selected = app->getModuleEditor()->getSelectedGameObject();
    ImGui::BeginDisabled(selected == nullptr);

    if (ImGui::Button("Remove") && selected)
    {
        CommandRemoveGameObject(scene, selected).run();
    }

    ImGui::EndDisabled();
}

void WindowHierarchy::drawPrefabHeader(PrefabEditSession* session)
{
    PrefabUI::drawModeHeader(session->m_sourcePath.filename().string().c_str());

    Scene* isolatedScene = session->m_isolatedScene;

    if (ImGui::Button("Add Child Object") && session->m_rootObject)
    {
        CommandAddChildToPrefabRoot(isolatedScene, session->m_rootObject).run();
    }

    ImGui::SameLine();

    GameObject* selected = app->getModuleEditor()->getSelectedGameObject();
    ImGui::BeginDisabled(selected == nullptr || selected == session->m_rootObject);

    if (ImGui::Button("Remove Selected") && selected && selected != session->m_rootObject)
    {
        CommandRemoveGameObject(isolatedScene, selected).run();
    }

    ImGui::EndDisabled();
}

void WindowHierarchy::drawSceneTree()
{
    Scene* scene = app->getModuleScene()->getScene();

    if (ImGui::TreeNodeEx(scene->getName()))
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
            {
                onReparent(*static_cast<GameObject**>(p->Data), nullptr);
            }

            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("PREFAB_ASSET"))
            {
                onPrefabDropOnNode(std::filesystem::path(static_cast<const char*>(p->Data)), nullptr);
            }

            ImGui::EndDragDropTarget();
        }

        for (GameObject* go : scene->getRootObjects())
        {
            m_treeRenderer.renderNode(go, false, m_selectionState);
        }

        ImGui::TreePop();
    }
}

void WindowHierarchy::drawPrefabTree(PrefabEditSession* session)
{
    if (!session->m_isolatedScene)
    {
        return;
    }

    for (GameObject* go : session->m_isolatedScene->getRootObjects())
    {
        m_treeRenderer.renderNode(go, true, m_selectionState);
    }
}

void WindowHierarchy::startRename(GameObject* target)
{
    if (!target)
    {
        return;
    }

    m_renameTargetID = target->GetID();
    m_renameFocusPending = true;
    strncpy_s(m_renameBuffer, sizeof(m_renameBuffer), target->GetName().c_str(), sizeof(m_renameBuffer) - 1);
}

void WindowHierarchy::drawInlineRename()
{
    if (m_renameTargetID == 0)
    {
        return;
    }

    ImGui::OpenPopup("##HierarchyRename");
    ImGui::SetNextWindowSize(ImVec2(260, 0), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("##HierarchyRename", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::Text("Rename object:");

        if (m_renameFocusPending)
        {
            ImGui::SetKeyboardFocusHere();
            m_renameFocusPending = false;
        }

        const bool commit = ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        const bool okClicked = ImGui::Button("OK");
        const bool cancelled = ImGui::IsKeyPressed(ImGuiKey_Escape);

        if ((commit || okClicked) && strlen(m_renameBuffer) > 0)
        {
            Scene* scene = HierarchyUtils::resolveTargetScene();
            if (GameObject* go = HierarchyUtils::findByUID(scene, m_renameTargetID))
            {
                CommandRenameGameObject(scene, go, m_renameBuffer).run();
            }

            m_renameTargetID = 0;
            ImGui::CloseCurrentPopup();
        }
        else if (cancelled)
        {
            m_renameTargetID = 0;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void WindowHierarchy::reparent(GameObject* child, GameObject* newParent)
{
    Scene* scene = HierarchyUtils::resolveTargetScene();
    CommandReparent(scene, child, newParent).run();
}

void WindowHierarchy::onSelect(GameObject* go)
{
    app->getModuleEditor()->setSelectedGameObject(go);
}

void WindowHierarchy::onReparent(GameObject* child, GameObject* newParent)
{
    reparent(child, newParent);
}

void WindowHierarchy::onPrefabDropOnNode(const std::filesystem::path& sourcePath, GameObject* parent)
{
    Scene* scene = HierarchyUtils::resolveTargetScene();
    CommandInstantiatePrefab(scene, sourcePath, parent).run();
}

void WindowHierarchy::onDeleteRequested(GameObject* go)
{
    if (!go)
    {
        return;
    }

    PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();

    if (session && session->m_active && go == session->m_rootObject)
    {
        return;
    }

    Scene* scene = HierarchyUtils::resolveTargetScene();
    CommandRemoveGameObject(scene, go).run();
}
