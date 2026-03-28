#include "Globals.h"
#include "WindowInspector.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include "PrefabUI.h"

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

    const bool prefabMode = app->getModuleEditor()->isInPrefabEditMode();;

    if (prefabMode)
    {
        PrefabUI::drawModeHeader(app->getModuleEditor()->getPrefabEditSourcePath().stem().string().c_str());
        PrefabUI::drawApplyRevertBar(ImGui::GetContentRegionAvail().x);
    }

    GameObject* selectedGameObject = app->getModuleEditor()->getSelectedGameObject();
    if (selectedGameObject)
    {
        selectedGameObject->drawUI();
    }

    ImGui::End();
}
