#include "Globals.h"
#include "WindowInspector.h"
#include "Application.h";
#include "ModuleEditor.h"
#include "GameObject.h"
#include "PrefabUI.h"
#include "PrefabEditSession.h"

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
        PrefabUI::drawModeHeader(session->m_prefabName.c_str());
        PrefabUI::drawApplyRevertBar(ImGui::GetContentRegionAvail().x);
    }

    GameObject* selectedGameObject = app->getModuleEditor()->getSelectedGameObject();
    if (selectedGameObject)
    {
        selectedGameObject->drawUI();

        if (!prefabMode)
            PrefabUI::drawSavePrefabSection(selectedGameObject);
    }

    ImGui::End();
}
