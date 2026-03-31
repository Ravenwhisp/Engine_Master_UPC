#include "Globals.h"
#include "WindowInspector.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include "PrefabUI.h"
#include "PrefabEditSession.h"
#include <ScriptFactory.h>
#include <ScriptComponent.h>

WindowInspector::WindowInspector()
{

}

void WindowInspector::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr(),
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
    const bool prefabMode = session && session->m_active;

    if (prefabMode)
    {
        PrefabUI::drawModeHeader(session->m_sourcePath.stem().string().c_str());
        PrefabUI::drawApplyRevertBar(ImGui::GetContentRegionAvail().x);
    }

    GameObject* selectedGameObject = app->getModuleEditor()->getSelectedGameObject();
    if (selectedGameObject)
    {
        selectedGameObject->drawUI();
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

    ImGui::End();
}
