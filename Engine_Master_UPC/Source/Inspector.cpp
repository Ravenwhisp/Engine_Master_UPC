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
    editorTransform = new EditorTransform();
    editorMeshRenderer = new EditorMeshRenderer();
    editorLight = new LightEditor();
}

void Inspector::Render()
{
    if (!ImGui::Begin(GetWindowName(), GetOpenPtr(),
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }


    if (_selectedGameObject) {
        bool isActive = true;
        if (ImGui::Checkbox("##Active", &isActive))
        {
            
        }
        ImGui::SameLine();

        // Name field
        char nameBuffer[256];
        strcpy_s(nameBuffer, _selectedGameObject->GetName());
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 80);
        if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer)))
        {
            _selectedGameObject->SetName(nameBuffer);
        }
        ImGui::PopItemWidth();

        // Tag and Layer (placeholder for now)
        ImGui::Spacing();
        ImGui::Text("Tag: Untagged");
        ImGui::SameLine(ImGui::GetWindowWidth() * 0.5f);
        ImGui::Text("Layer: Default");

        //THIS IS PROVISIONAL
        auto transform = _selectedGameObject->GetComponent<Transform>();
        if (transform && ImGui::CollapsingHeader(editorTransform->GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            editorTransform->Render();
            ImGui::Unindent();
        }

        auto model = _selectedGameObject->GetComponent<Emeika::Model>();
        if (model && ImGui::CollapsingHeader(editorMeshRenderer->GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            editorMeshRenderer->Render();
            ImGui::Unindent();
        }

        auto light = _selectedGameObject->GetComponent<Light>();
        if (light && ImGui::CollapsingHeader(editorLight->GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            editorLight->Render();
            ImGui::Unindent();
        }
    }

    ImGui::End();
}

void Inspector::SetSelectedGameObject(GameObject* gameObject)
{
    _selectedGameObject = gameObject;
    editorTransform->SetGameObject(_selectedGameObject);
    editorMeshRenderer->SetGameObject(_selectedGameObject);
    editorLight->SetGameObject(_selectedGameObject);
}
