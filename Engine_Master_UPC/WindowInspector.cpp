#include "Globals.h"
#include "WindowInspector.h"
#include "Application.h";
#include "ModuleEditor.h"
#include "GameObject.h"

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

    GameObject* selectedGameObject = app->getModuleEditor()->getSelectedGameObject();
    if (selectedGameObject) 
    {
        selectedGameObject->drawUI();
    }

    ImGui::End();
}
