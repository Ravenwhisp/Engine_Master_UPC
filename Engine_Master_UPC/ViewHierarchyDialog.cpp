#include "Globals.h"
#include "ViewHierarchyDialog.h"

#include "Application.h"
#include "EditorModule.h"
#include "SceneModule.h"

#include "GameObject.h"

ViewHierarchyDialog::ViewHierarchyDialog()
{
	m_editorModule = app->getEditorModule();
    m_sceneModule = app->getSceneModule();
}

void ViewHierarchyDialog::render()
{
    DEBUG_LOG("Supuestamente abierto el popup");
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
            m_sceneModule->removeGameObject(selected->GetID());
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Create Empty"))
    {
        app->getSceneModule()->createGameObject();
    }

    if (ImGui::MenuItem("Create Camera"))
    {
        //app->getSceneModule()->createCamera();
    }

    if (ImGui::BeginMenu("Create Light"))
    {
        if (ImGui::MenuItem("Point"))
        {
            //app->getSceneModule()->createPointLight();
        }

        if (ImGui::MenuItem("Directional"))
        {
            //app->getSceneModule()->createDirectionalLight();
        }

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Create Model"))
    {

    }
}