#include "Globals.h"
#include "Inspector.h"
#include "Application.h";
#include "EditorModule.h"
#include "GameObject.h"
#include "PrefabUI.h"
#include "PrefabEditSession.h"

Inspector::Inspector()
{

}

void Inspector::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr(),
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    PrefabEditSession* session = app->getEditorModule()->getPrefabSession();
    const bool         prefabMode = session && session->active;

    if (prefabMode)
    {
        PrefabUI::drawModeHeader(session->prefabName.c_str());
        PrefabUI::drawApplyRevertBar(ImGui::GetContentRegionAvail().x);
    }

    GameObject* selectedGameObject = app->getEditorModule()->getSelectedGameObject();
    if (selectedGameObject)
    {
        if (!prefabMode)
            PrefabUI::drawInstanceBadge(selectedGameObject);

        selectedGameObject->drawUI();

        if (!prefabMode)
            PrefabUI::drawSavePrefabSection(selectedGameObject);
    }

    ImGui::End();
}
