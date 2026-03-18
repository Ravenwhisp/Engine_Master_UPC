#include "Globals.h"
#include "WindowSceneEditor.h"
#include "ImGuizmo.h"
#include <imgui.h>

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleEditor.h"
#include "ModuleCamera.h"
#include "ModuleNavigation.h"
#include "PrefabUI.h"

#include "ModuleRender.h"
#include "ModuleScene.h"
#include "EditorToolbar.h"
#include "PlayToolbar.h"

#include "Settings.h"

#include "GameObject.h"
#include "DebugDrawPass.h"
#include "LightDebugDraw.h"
#include "LightComponent.h"
#include "TriggerArea.h"
#include "NavigationAgentComponent.h"
#include "Quadtree.h"
#include "Transform.h"

#include "CameraComponent.h"
#include <WindowLogger.h>


static bool ScreenToWorldOnPlaneY0(
    const ImVec2& mousePos,
    const ImVec2& vpPos,
    const ImVec2& vpSize,
    const Matrix& view,
    const Matrix& proj,
    Vector3& outWorld)
{
    if (vpSize.x <= 1 || vpSize.y <= 1) return false;

    // mouse -> viewport UV [0..1]
    float u = (mousePos.x - vpPos.x) / vpSize.x;
    float v = (mousePos.y - vpPos.y) / vpSize.y;
    if (u < 0 || u > 1 || v < 0 || v > 1) return false;

    // UV -> NDC (D3D: z=0..1). Ojo con Y invertida.
    float ndcX = 2.0f * u - 1.0f;
    float ndcY = 1.0f - 2.0f * v;

    Matrix invViewProj = (view * proj).Invert();

    Vector4 nearClip(ndcX, ndcY, 0.0f, 1.0f);
    Vector4 farClip(ndcX, ndcY, 1.0f, 1.0f);

    Vector4 nearWorld4 = Vector4::Transform(nearClip, invViewProj);
    Vector4 farWorld4 = Vector4::Transform(farClip, invViewProj);

    if (nearWorld4.w == 0 || farWorld4.w == 0) return false;

    Vector3 nearWorld(nearWorld4.x / nearWorld4.w, nearWorld4.y / nearWorld4.w, nearWorld4.z / nearWorld4.w);
    Vector3 farWorld(farWorld4.x / farWorld4.w, farWorld4.y / farWorld4.w, farWorld4.z / farWorld4.w);

    Vector3 dir = farWorld - nearWorld;
    dir.Normalize();

    if (fabsf(dir.y) < 1e-5f) return false;
    float t = (0.0f - nearWorld.y) / dir.y;
    if (t < 0.0f) return false;

    outWorld = nearWorld + dir * t;
    return true;
}

static void DebugDrawWindowHierarchy(GameObject* go)
{
    if (!go || !go->GetActive())
        return;

    // --- Lights  ---
    if (auto* light = go->GetComponentAs<LightComponent>(ComponentType::LIGHT))
    {
        if (light->isDebugDrawEnabled())
        {
            if (light->isDebugDrawDepthEnabled())
                LightDebugDraw::drawLightWithDepth(*go);
            else
                LightDebugDraw::drawLightWithoutDepth(*go);
        }
    }

    // --- Navigation Agent path ---
    if (auto* agent = go->GetComponentAs<NavigationAgentComponent>(ComponentType::NAVIGATION_AGENT))
    {
        if (agent->isActive())
            agent->drawDebugPath();
    }

    // recurse children
    for (GameObject* child : go->GetTransform()->getAllChildren())
        DebugDrawWindowHierarchy(child);
}

WindowSceneEditor::WindowSceneEditor()
{
    m_moduleCamera = app->getModuleCamera();
    m_moduleInput = app->getModuleInput();

    m_settings = app->getSettings();

    m_editorToolbar = new EditorToolbar();
	m_playToolbar = new PlayToolbar();

    auto d3d12Module = app->getModuleD3D12();
}

WindowSceneEditor::~WindowSceneEditor()
{
    delete m_editorToolbar;
	delete m_playToolbar;
}

void WindowSceneEditor::update()
{

}

void WindowSceneEditor::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr(), ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    float toolbarWidth = ImGui::GetContentRegionAvail().x;
	m_playToolbar->DrawCentered(toolbarWidth);
    ImGui::NewLine();
    m_editorToolbar->DrawCentered(toolbarWidth);
    ImGui::NewLine();
    ImGui::Separator();

    ImVec2 windowPos = ImGui::GetWindowPos();
    m_windowX = windowPos.x;
    m_windowY = windowPos.y;

    ImVec2 contentRegion = ImGui::GetContentRegionAvail();

    if (contentRegion.x > 0 && contentRegion.y > 0) 
    {
        resize(contentRegion);

        ImVec2 imageTopLeft = ImGui::GetCursorScreenPos();
        m_viewportX = imageTopLeft.x;
        m_viewportY = imageTopLeft.y;

        ImTextureID textureID = (ImTextureID)app->getModuleRender()->getGPUEditorScreenRT().ptr;
        ImGui::Image(textureID, m_size);
        
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    ImVec2 contentPos(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
    ImVec2 contentSize(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

    m_viewportPos = contentPos;

    ImGuizmo::SetRect(contentPos.x, contentPos.y, contentSize.x, contentSize.y);
    ImGuizmo::Enable(true);

    GameObject* selectedGameObject = app->getModuleEditor()->getSelectedGameObject();

    if (selectedGameObject && m_moduleCamera)
    {
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        bool shouldShowGizmo = m_settings->sceneEditor.showGuizmo;

        ModuleEditor::SCENE_TOOL currentMode = app->getModuleEditor()->getCurrentSceneTool();

        switch (currentMode) 
        {
        case ModuleEditor::SCENE_TOOL::MOVE:          op = ImGuizmo::TRANSLATE; break;
        case ModuleEditor::SCENE_TOOL::ROTATE:        op = ImGuizmo::ROTATE; break;
        case ModuleEditor::SCENE_TOOL::SCALE:         op = ImGuizmo::SCALE; break;
        case ModuleEditor::SCENE_TOOL::TRANSFORM:     op = ImGuizmo::UNIVERSAL; break;
        default: shouldShowGizmo = false; break;
        }

        if (shouldShowGizmo) 
        {
            Transform* transform = selectedGameObject->GetTransform();
            Matrix worldMatrix = transform->getGlobalMatrix();

            ImGuizmo::MODE gizmoMode = app->getModuleEditor()->isGizmoLocal() ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

            ImGuizmo::Manipulate(
                (float*)&m_moduleCamera->getView(),
                (float*)&m_moduleCamera->getProjection(),
                op,
                gizmoMode,
                (float*)&worldMatrix
            );

            if (ImGuizmo::IsUsing())
            {
                transform->setFromGlobalMatrix(worldMatrix);
                PrefabUI::markTransformOverride(selectedGameObject);
            }
        }
    }

    m_isViewportHovered = ImGui::IsWindowHovered();
    m_isViewportFocused = ImGui::IsWindowFocused();

    ImGui::End();
}

bool WindowSceneEditor::resize(ImVec2 contentRegion)
{
    if (abs(contentRegion.x - m_size.x) > 1.0f ||
        abs(contentRegion.y - m_size.y) > 1.0f) 
    {
        setSize(contentRegion);
        return true;
    }
    return false;
}

void WindowSceneEditor::renderDebugDrawPass(ID3D12GraphicsCommandList* commandList)
{
    if (m_settings->sceneEditor.showGrid)
    {
        dd::xzSquareGrid(-10.0f, 10.f, 0.0f, 1.0f, dd::colors::LightGray);
    }

    if (m_settings->sceneEditor.showAxis)
    {
        dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);
    }

    if (m_settings->sceneEditor.showQuadTree)
    {
        renderQuadtree();
    }

    for (GameObject* root : app->getModuleScene()->getAllGameObjects()) 
    {
        DebugDrawWindowHierarchy(root);
    }
        
    ModuleNavigation* nav = app->getModuleNavigation();
    if (nav && nav->getDrawNavMesh() && nav->getNavMesh())
    {
        const auto& lines = nav->getNavMeshDebugLines();
        for (const auto& l : lines) 
        {
            dd::line(ddConvert(l.a), ddConvert(l.b), dd::colors::Green);
        } 
    }

    if (nav && nav->hasDebugPath())
    {
        const auto& pts = nav->getDebugPathPoints();
        for (size_t i = 1; i < pts.size(); ++i)
            dd::line(ddConvert(pts[i - 1]), ddConvert(pts[i]), dd::colors::Yellow);

        // marks start/end
        dd::line(ddConvert(pts.front()), ddConvert(pts.front() + Vector3(0, 0.25f, 0)), dd::colors::Yellow);
        dd::line(ddConvert(pts.back()), ddConvert(pts.back() + Vector3(0, 0.25f, 0)), dd::colors::Yellow);
    }

    if (m_settings->sceneEditor.showModelBoundingBoxes)
    {
        for (const auto& renderer : app->getModuleScene()->getAllMeshRenderers())
        {
            drawBoundingBox(renderer->getBoundingBox(), dd::colors::Yellow);
        }
    }


    Matrix viewMatrix;
    Matrix projectionMatrix;

    if (app->getCurrentCameraPerspective())
    {
        viewMatrix = app->getCurrentCameraPerspective()->getViewMatrix();
        projectionMatrix = app->getCurrentCameraPerspective()->getProjectionMatrix();
    }
    else
    {
        viewMatrix = app->getModuleCamera()->getView();
        projectionMatrix = app->getModuleCamera()->getProjection();
    }

    // Mouse Path tool
    if (m_isViewportHovered)
    {
        Vector3 hit;
        ImVec2 mouse = ImGui::GetMousePos();

        // Start - left click
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            if (ScreenToWorldOnPlaneY0(mouse, m_viewportPos, getSize(), viewMatrix, projectionMatrix, hit))
            {
                app->getModuleNavigation()->setPathStart(hit);
                LOG_INFO(__FILE__, __LINE__, "Pick start: %.2f %.2f %.2f", hit.x, hit.y, hit.z);
            }
                
        }

        // end - right click
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            if (ScreenToWorldOnPlaneY0(mouse, m_viewportPos, getSize(), viewMatrix, projectionMatrix, hit))
            {
                app->getModuleNavigation()->setPathEnd(hit);
                LOG_INFO(__FILE__, __LINE__, "Pick end: %.2f %.2f %.2f", hit.x, hit.y, hit.z);
            }
                
        }
    }

}

void WindowSceneEditor::renderQuadtree()
{
    Quadtree* quadtree = app->getModuleScene()->getQuadtree();
    if (!quadtree)
    {
        return;
    }

    std::vector<BoundingRect> quadrants = quadtree->getQuadrants();
    for (const auto& rect : quadrants)
    {
        Vector3 extents(rect.width * 0.5f, 0.0f, rect.height * 0.5f);
		Vector3 center(rect.x + rect.width * 0.5f, 0.1f, rect.y + rect.height * 0.5f);

        
        float color[3];
        if (rect.m_debugIsCulled) 
        {
        dd::box(ddConvert(center), dd::colors::Red, extents.x * 2.0f, extents.y * 2.0f, extents.z * 2.0f);
	}
        else 
        {
            dd::box(ddConvert(center), dd::colors::Green, extents.x * 2.0f, extents.y * 2.0f, extents.z * 2.0f);
        }

        int minY = -10000;
        int maxY = 10000;

        center.y = (minY + maxY) * 0.5f;

        extents.y = (maxY - minY) * 0.5f;

        Vector3 min = center - extents;
        Vector3 max = center + extents;

        Vector3 bbPoints[8] =
        {
            Vector3(center.x - extents.x, center.y - extents.y, center.z - extents.z),
            Vector3(center.x - extents.x, center.y - extents.y, center.z + extents.z),
            Vector3(center.x - extents.x, center.y + extents.y, center.z - extents.z),
            Vector3(center.x - extents.x, center.y + extents.y, center.z + extents.z),
            Vector3(center.x + extents.x, center.y - extents.y, center.z - extents.z),
            Vector3(center.x + extents.x, center.y - extents.y, center.z + extents.z),
            Vector3(center.x + extents.x, center.y + extents.y, center.z - extents.z),
            Vector3(center.x + extents.x, center.y + extents.y, center.z + extents.z)
        };

        Engine::BoundingBox bb = Engine::BoundingBox(min, max, bbPoints);
        bb.render();
	}
}

void WindowSceneEditor::drawBoundingBox(const Engine::BoundingBox& bbox, const ddVec3& color)
{
    const Vector3* c = bbox.getPoints();

    ddVec3 pts[8];

    for (int i = 0; i < 8; ++i)
    {
        pts[i][0] = c[i].x;
        pts[i][1] = c[i].y;
        pts[i][2] = c[i].z;
    }

    dd::box(pts, color, 0, false);
}