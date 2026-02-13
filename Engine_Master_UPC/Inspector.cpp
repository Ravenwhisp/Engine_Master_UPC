#include "Globals.h"
#include "Inspector.h"
#include "Application.h";
#include "EditorModule.h"
#include "GameObject.h"

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

    GameObject* selectedGameObject = app->getEditorModule()->getSelectedGameObject();
    if (selectedGameObject) {
        selectedGameObject->drawUI();
    }

    ImGui::End();
}
