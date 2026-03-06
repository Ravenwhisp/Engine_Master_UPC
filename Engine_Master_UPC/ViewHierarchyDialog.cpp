#include "Globals.h"
#include "ViewHierarchyDialog.h"

#include "Application.h"
#include "EditorModule.h"
#include "SceneModule.h"

#include "GameObject.h"
#include "Hierarchy.h"
#include <LightComponent.h>

ViewHierarchyDialog::ViewHierarchyDialog(Hierarchy* hierarchy)
{
	m_editorModule = app->getEditorModule();
    m_sceneModule = app->getSceneModule();

	m_hierarchy = hierarchy;
}

void ViewHierarchyDialog::render()
{
    GameObject* selected = m_editorModule->getSelectedGameObject();
    bool hasSelection = selected != nullptr;

    if (ImGui::MenuItem("Remove", nullptr, false, hasSelection))
    {
        if (hasSelection)
        {
            m_editorModule->setSelectedGameObject(nullptr);
            m_sceneModule->removeGameObject(selected->GetID());
        }
    }

    if (ImGui::MenuItem("Rename", nullptr, false, hasSelection))
    {
        if (hasSelection)
        {
			m_hierarchy->startRename(selected);
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Copy", nullptr, false, hasSelection))
    {
        DEBUG_WARN("Option not implemented yet!");
        //missing code to copy game objects
    }

    if (ImGui::MenuItem("Paste", nullptr, false, hasSelection))
    {
        DEBUG_WARN("Option not implemented yet!");
        //missing code to paste game objects
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Create Empty"))
    {
        app->getSceneModule()->createGameObject();
    }

    if (ImGui::MenuItem("Create Camera"))
    {
		GameObject* camera = app->getSceneModule()->createGameObject();
		camera->AddComponent(ComponentType::CAMERA);
    }

    if (ImGui::BeginMenu("Create Light"))
    {
        if (ImGui::MenuItem("Point"))
        {
            GameObject* light = app->getSceneModule()->createGameObject();
            LightComponent* lightComp = (LightComponent*)light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID());
            lightComp->setTypePoint(10.f);
        }

        if (ImGui::MenuItem("Directional"))
        {
            GameObject* light = app->getSceneModule()->createGameObject();
            LightComponent* lightComp = (LightComponent*)light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID());
            lightComp->setTypeDirectional();
        }

        if (ImGui::MenuItem("Spot"))
        {
            GameObject* light = app->getSceneModule()->createGameObject();
            LightComponent* lightComp = (LightComponent*)light->AddComponentWithUID(ComponentType::LIGHT, GenerateUID());
            lightComp->setTypeSpot(10.f, 20.f, 30.f);
        }

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Create Model"))
    {
        DEBUG_WARN("Option not implemented yet!");
		//missing code to create model
    }
}