#include "Globals.h"
#include "EditorModule.h"
#include "D3D12Module.h"
#include "CameraModule.h"
#include "vector"
#include <backends/imgui_impl_dx12.h>
#include "Resources.h"
#include "SceneEditor.h"
#include "HardwareWindow.h"
#include "PerformanceWindow.h"
#include "EditorWindow.h"
#include "ImGuizmo.h"
#include "Logger.h"
#include "ImGuiPass.h"
#include "Hierarchy.h"
#include "Inspector.h"

using namespace std;

Logger* logger = nullptr;

// In the future this will be also a EditorWindow
void mainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}


void style() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding = 4.0f;

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.105f, 0.11f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);

}


void EditorModule::mainDockspace(bool* p_open)
{
    // Fullscreen window flags
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiViewportFlags_NoAutoMerge |
        ImGuiWindowFlags_NoNavFocus;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("MainDockspaceWindow", p_open, window_flags);

    ImGui::PopStyleVar(2);

    mainMenuBar();
    // Create the DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_None);

    if (m_firstFrame) 
    {
        for (auto it = m_editorWindows.begin(); it != m_editorWindows.end(); ++it)
            (*it)->render();

        setupDockLayout(dockspace_id);
        style();
        m_firstFrame = false;
    }

    ImGui::End();
}


void EditorModule::setupDockLayout(ImGuiID dockspace_id)
{

    // Clear previous layout
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID dock_main = dockspace_id;

    ImGuiID dock_left, dock_inspector;
    ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.75f, &dock_left, &dock_inspector);

    ImGuiID dock_bottom, dock_top;
    ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_top);

    ImGuiID dock_hierarchy, dock_scene;
    ImGui::DockBuilderSplitNode(dock_top, ImGuiDir_Left, 0.25f, &dock_hierarchy, &dock_scene);

    ImGui::DockBuilderDockWindow("Inspector", dock_inspector);
    ImGui::DockBuilderDockWindow("Hierarchy", dock_hierarchy);
    ImGui::DockBuilderDockWindow("Scene Editor", dock_scene);

    ImGui::DockBuilderDockWindow("Console", dock_bottom);
    ImGui::DockBuilderDockWindow("Hardware Info", dock_bottom);
    ImGui::DockBuilderDockWindow("Performance", dock_bottom);

    ImGui::DockBuilderFinish(dockspace_id);
}

EditorModule::EditorModule()
{
    //_console = Console();
    m_editorWindows.push_back(m_logger = new Logger());
    m_editorWindows.push_back(m_hardwareWindow = new HardwareWindow());
    m_editorWindows.push_back(m_performanceWindow = new PerformanceWindow());
}

EditorModule::~EditorModule()
{
    m_gui->~ImGuiPass();
    //_sceneView->~SceneView();
    m_debugDrawPass->~DebugDrawPass();
}

bool EditorModule::postInit()
{
	D3D12Module* _d3d12 = app->getD3D12Module();
    m_gui = new ImGuiPass(_d3d12->getDevice(), _d3d12->getWindowHandle(),
        app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getCPUHandle(0), 
        app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getGPUHandle(0));    
    SceneEditor* scene = new SceneEditor();
    m_sceneView = scene;
    m_editorWindows.push_back(scene);
    
    Hierarchy* hierarchy = new Hierarchy();
    Inspector* inspector = new Inspector();
    hierarchy->setOnSelectedGameObject([inspector](GameObject* g) { inspector->setSelectedGameObject(g); });
    hierarchy->setOnSelectedGameObject([scene](GameObject* g) { scene->setSelectedGameObject(g); });

    m_editorWindows.push_back(hierarchy);
    m_editorWindows.push_back(inspector);

	return true;
}

void EditorModule::update()
{
    for (auto it = m_editorWindows.begin(); it != m_editorWindows.end(); ++it)
        (*it)->update();
}

void EditorModule::preRender()
{
    m_gui->startFrame();
    ImGuizmo::BeginFrame();
    
    mainMenuBar();
    mainDockspace(&m_showMainDockspace);

    for (auto it = m_editorWindows.begin(); it != m_editorWindows.end(); ++it)
        (*it)->render();

    ImGui::EndFrame();
}

void EditorModule::render()
{
    auto commandList = app->getD3D12Module()->getCommandList();
    m_gui->record(commandList);
}

void EditorModule::postRender()
{
}

bool EditorModule::cleanUp()
{
    for (auto it = m_editorWindows.begin(); it != m_editorWindows.end(); ++it)
        (*it)->cleanUp();
    return true;
}
