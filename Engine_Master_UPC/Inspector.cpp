#include "Globals.h"
#include "Inspector.h"
#include "GameObject.h"

#include "Transform.h"
#include "Model.h"
#include "Light.h"


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


    if (m_selectedGameObject) {
        m_selectedGameObject->drawUI();
    }

    ImGui::End();
}

void Inspector::setSelectedGameObject(GameObject* gameObject)
{
    m_selectedGameObject = gameObject;
}
