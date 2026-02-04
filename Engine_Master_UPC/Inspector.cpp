#include "Globals.h"
#include "Inspector.h"
#include "GameObject.h"

#include "Transform.h"
#include "Model.h"
#include "Light.h"

#include "EditorTransform.h"
#include "EditorMeshRenderer.h"
#include "LightEditor.h"

Inspector::Inspector()
{
    m_editorTransform = new EditorTransform();
    m_editorMeshRenderer = new EditorMeshRenderer();
    m_editorLight = new LightEditor();
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
        bool isActive = true;

        if (ImGui::Checkbox("##Active", &isActive))
        {
            
        }

        ImGui::SameLine();

        // Name field
        char nameBuffer[256];
        strcpy_s(nameBuffer, m_selectedGameObject->GetName());
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 80);
        if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer)))
        {
            m_selectedGameObject->SetName(nameBuffer);
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Text("Tag: Untagged");
        ImGui::SameLine(ImGui::GetWindowWidth() * 0.5f);
        ImGui::Text("Layer: Default");


        auto transform = m_selectedGameObject->GetComponent<Transform>();
        if (transform && ImGui::CollapsingHeader(m_editorTransform->getName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            m_editorTransform->render();
            ImGui::Unindent();
        }

        auto model = m_selectedGameObject->GetComponent<Emeika::Model>();
        if (model && ImGui::CollapsingHeader(m_editorMeshRenderer->getName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            m_editorMeshRenderer->render();
            ImGui::Unindent();
        }

        auto light = m_selectedGameObject->GetComponent<Light>();
        if (light && ImGui::CollapsingHeader(m_editorLight->getName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            m_editorLight->render();
            ImGui::Unindent();
        }
    }

    ImGui::End();
}

void Inspector::setSelectedGameObject(GameObject* gameObject)
{
    m_selectedGameObject = gameObject;
    m_editorTransform->setGameObject(m_selectedGameObject);
    m_editorMeshRenderer->setGameObject(m_selectedGameObject);
    m_editorLight->setGameObject(m_selectedGameObject);
}
