#include "Globals.h"
#include "WindowInspector.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "GameObject.h"
#include "Scene.h"
#include "PrefabUI.h"
#include "PrefabEditSession.h"
#include <ScriptFactory.h>
#include <ScriptComponent.h>

WindowInspector::WindowInspector()
{
}

void WindowInspector::lockInspector(GameObject* go)
{
    m_isLocked = true;
    m_lockedGameObjectUID = go ? go->GetID() : 0;
}

void WindowInspector::unlockInspector()
{
    m_isLocked = false;
    m_lockedGameObjectUID = 0;
}

bool WindowInspector::isLocked() const
{
    return m_isLocked;
}

void WindowInspector::drawInternal()
{
    PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
    const bool prefabMode = session && session->m_active;

    if (prefabMode)
    {
        PrefabUI::drawModeHeader(session->m_sourcePath.stem().string().c_str());
        PrefabUI::drawApplyRevertBar(ImGui::GetContentRegionAvail().x);
    }

    ImGui::SameLine();
    if (ImGui::Button(m_isLocked ? "Unlock" : "Lock"))
    {
        if (m_isLocked)
        {
            unlockInspector();
        }
        else
        {
            GameObject* selected = app->getModuleEditor()->getSelectedGameObject();
            if (selected)
            {
                lockInspector(selected);
            }
        }
    }

    GameObject* displayObject = nullptr;

    if (m_isLocked && m_lockedGameObjectUID != 0)
    {
        Scene* scene = app->getModuleScene()->getScene();
        displayObject = scene->findGameObjectByUID(m_lockedGameObjectUID);
        if (!displayObject)
        {
            unlockInspector();
        }
    }
    else
    {
        displayObject = app->getModuleEditor()->getSelectedGameObject();
    }

    if (displayObject)
    {
        displayObject->drawUI();
    }

    ImGui::Spacing();
    ImGui::Separator();

    const bool isDragHovering = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::GetDragDropPayload() && ImGui::GetDragDropPayload()->IsDataType("SCRIPT_ASSET");

    if (isDragHovering)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.35f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.45f, 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.28f, 0.55f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.90f, 1.00f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.50f, 1.0f));
    }

    const float zoneWidth = ImGui::GetContentRegionAvail().x;
    ImGui::Button("  [S]  Drop script asset to add ScriptComponent  ", ImVec2(zoneWidth, 34.0f));

    ImGui::PopStyleColor(4);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCRIPT_ASSET"))
        {
            const char* scriptName = static_cast<const char*>(payload->Data);
            GameObject* selectedGameObject = app->getModuleEditor()->getSelectedGameObject();

            if (scriptName && selectedGameObject && ScriptFactory::isScriptRegistered(scriptName))
            {
                Component* comp = selectedGameObject->AddComponentWithUID(ComponentType::SCRIPT, GenerateUID());

                if (comp)
                {
                    ScriptComponent* sc = static_cast<ScriptComponent*>(comp);
                    sc->setScriptName(scriptName);
                    sc->createScriptInstance();
                }
            }
            else if (scriptName && selectedGameObject && !ScriptFactory::isScriptRegistered(scriptName))
            {
                DEBUG_WARN("[Inspector] Script '%s' is not registered in ScriptFactory. Ensure the DLL exports it correctly.", scriptName);
            }
        }
        ImGui::EndDragDropTarget();
    }

}
