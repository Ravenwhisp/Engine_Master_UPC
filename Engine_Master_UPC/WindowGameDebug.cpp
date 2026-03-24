#include "Globals.h"
#include "WindowGameDebug.h"

#include "imgui.h"

#include "Application.h"
#include "Settings.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "NavMeshWalk.h"

WindowGameDebug::WindowGameDebug()
{
    m_settings = app->getSettings();
}


void WindowGameDebug::render()
{
    ImGui::Begin("Debug (F3 to close)");

    ImGui::Text("Engine version %s", m_settings->engine.version.c_str());
    
    ImGui::Separator();
    ImGui::Checkbox("Show Grid", &m_settings->sceneEditor.showGrid);
    ImGui::Checkbox("Show Axis", &m_settings->sceneEditor.showAxis);
    ImGui::Checkbox("Show QuadTree", &m_settings->sceneEditor.showQuadTree);
    ImGui::Checkbox("Show Model Bounding Boxes", &m_settings->sceneEditor.showModelBoundingBoxes);

    ImGui::Separator();
    ImGui::Checkbox("Frustum culling", &m_settings->frustumCulling.debugFrustumCulling);

    //ImGui::Spacing();
    ImGui::Separator();
    ImGui::Checkbox("Show FPS", &m_settings->debugGame.showFPS);
    ImGui::Checkbox("Show Frame time", &m_settings->debugGame.showFrametime);
    ImGui::Checkbox("Show triangles number", &m_settings->debugGame.showTrianglesNumber);


    static bool s_initConstrain = false;
    static bool s_constrainAllNavMeshWalk = true;

    if (!s_initConstrain)
    {
       
        for (GameObject* go : app->getModuleScene()->getScene()->getAllGameObjects())
        {
            if (!go) continue;
            if (auto* n = go->GetComponentAs<NavMeshWalk>(ComponentType::NAVMESH_WALK))
            {
                s_constrainAllNavMeshWalk = n->getNavmeshConstrain();
                break;
            }
        }
        s_initConstrain = true;
    }

    if (ImGui::Checkbox("Constrain NavMeshWalk to NavMesh", &s_constrainAllNavMeshWalk))
    {
        for (GameObject* go : app->getModuleScene()->getScene()->getAllGameObjects())
        {
            if (!go) continue;

            if (auto* n = go->GetComponentAs<NavMeshWalk>(ComponentType::NAVMESH_WALK))
            {
                n->setNavmeshConstrain(s_constrainAllNavMeshWalk);
            }
        }
    }

    ImGui::End();
}