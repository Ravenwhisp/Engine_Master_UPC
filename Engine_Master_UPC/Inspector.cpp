#include "Globals.h"
#include "Inspector.h"
#include "GameObject.h"

Inspector::Inspector() : m_selectedGameObject(nullptr)
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


    if (m_selectedGameObject) {
        m_selectedGameObject->drawUI();
    }

    ImGui::End();
}

void Inspector::setSelectedGameObject(GameObject* gameObject)
{
    m_selectedGameObject = gameObject;
}
