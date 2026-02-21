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
#include "EditorSettings.h"
#include "FileDialog.h"

#include "Application.h"
#include "SceneModule.h"

using namespace std;

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
        setupDockLayout(dockspace_id);
        style();
        m_firstFrame = false;
    } 

    ImGui::End();
}


void EditorModule::setupDockLayout(ImGuiID dockspace_id)
{
    // Clear previous layout
    ImGui::DockBuilderRemoveNodeDockedWindows(dockspace_id);
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
    ImGui::DockBuilderDockWindow("Editor Settings", dock_inspector);
    ImGui::DockBuilderDockWindow("Hierarchy", dock_hierarchy);
    ImGui::DockBuilderDockWindow("Editor Settings", dock_hierarchy);
    ImGui::DockBuilderDockWindow("Scene Editor", dock_scene);

    ImGui::DockBuilderDockWindow("FileDialog", dock_bottom);
    ImGui::DockBuilderDockWindow("Console", dock_bottom);
    ImGui::DockBuilderDockWindow("Hardware Info", dock_bottom);
    ImGui::DockBuilderDockWindow("Performance", dock_bottom);

    ImGui::DockBuilderFinish(dockspace_id);
}


bool EditorModule::init()
{
    m_editorWindows.push_back(m_logger = new Logger());
    m_editorWindows.push_back(m_hardwareWindow = new HardwareWindow());
    m_editorWindows.push_back(m_performanceWindow = new PerformanceWindow());
    m_editorWindows.push_back(m_editorSettings = new EditorSettings());
    m_editorWindows.push_back(new FileDialog());

	D3D12Module* _d3d12 = app->getD3D12Module();
    m_gui = new ImGuiPass(_d3d12->getDevice(), _d3d12->getWindowHandle(),
        app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getCPUHandle(0), 
        app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getGPUHandle(0));    

    m_sceneEditor = new SceneEditor();
    m_editorWindows.push_back(m_sceneEditor);
    
    Hierarchy* hierarchy = new Hierarchy();
    Inspector* inspector = new Inspector();

    m_editorWindows.push_back(hierarchy);
    m_editorWindows.push_back(inspector);

	return true;
}

void EditorModule::update()
{
    if (app->getEditorModule()->getSceneEditor()->isFocused())
    {
        handleKeyboardShortcuts();
    }

    for (auto it = m_editorWindows.begin(); it != m_editorWindows.end(); ++it)
    {
        (*it)->update();
    }
}

void EditorModule::preRender()
{
    m_gui->startFrame();
    ImGuizmo::BeginFrame();
    
    mainDockspace(&m_showMainDockspace);

    for (auto it = m_editorWindows.begin(); it != m_editorWindows.end(); ++it)
    {
        (*it)->render();
    }

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
    app->getD3D12Module()->getCommandQueue()->flush();

    for (auto window : m_editorWindows)
    {
        window->cleanUp();
    }

    for (auto window : m_editorWindows)
    {
        delete window;
    }
    m_editorWindows.clear();

    delete m_gui;
    m_gui = nullptr;

    delete m_debugDrawPass;
    m_debugDrawPass = nullptr;

    m_sceneEditor = nullptr;
    m_logger = nullptr;
    m_hardwareWindow = nullptr;
    m_performanceWindow = nullptr;

    return true;
}


void EditorModule::setSceneTool(SCENE_TOOL newTool) {
    if (currentSceneTool == newTool) {
        toggleGizmoMode();
        return;
    }

    currentSceneTool = newTool;
}

void EditorModule::setMode(SCENE_TOOL sceneTool, NAVIGATION_MODE navigationMode) {
    if (previousSceneTool == NONE) {
        previousSceneTool = currentSceneTool;
        currentSceneTool = sceneTool;
        currentNavigationMode = navigationMode;
    }
}

void EditorModule::resetMode() {
    if (previousSceneTool != NONE) {
        currentSceneTool = previousSceneTool;
        previousSceneTool = NONE;
    }
    currentNavigationMode = PAN;
}

void EditorModule::handleKeyboardShortcuts() {
    Keyboard::State keyboardState = Keyboard::Get().GetState();
    Mouse::State mouseState = Mouse::Get().GetState();

    DirectX::Mouse::ButtonStateTracker buttonStateTracker;
    buttonStateTracker.Update(mouseState);

    if (mouseState.rightButton) {
        setMode(NAVIGATION, FREE_LOOK);
    }
    else if (mouseState.leftButton && (keyboardState.LeftAlt || keyboardState.RightAlt)) {
        setMode(NAVIGATION, ORBIT);
    }
    else if (mouseState.middleButton) {
        setMode(NAVIGATION, PAN);
    }
    else if (!mouseState.leftButton) {
        resetMode();
        handleQWERTYCases(keyboardState);
    }
}

void EditorModule::handleQWERTYCases(Keyboard::State keyboardState) {
    static Keyboard::KeyboardStateTracker keyTracker;
    keyTracker.Update(keyboardState);

    if (keyTracker.pressed.W) setSceneTool(MOVE);
    if (keyTracker.pressed.E) setSceneTool(ROTATE);
    if (keyTracker.pressed.R) setSceneTool(SCALE);
    if (keyTracker.pressed.T) setSceneTool(RECT);
    if (keyTracker.pressed.Y) setSceneTool(TRANSFORM);

    if (keyTracker.pressed.Q) {
        currentSceneTool = NAVIGATION;
        currentNavigationMode = PAN;
    }
}
