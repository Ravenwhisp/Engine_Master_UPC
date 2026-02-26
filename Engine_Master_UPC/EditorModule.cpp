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
#include "SceneConfig.h"

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

    // --- Layout & Rounding ---
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;  // Unity uses very subtle rounding
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;
    style.ChildRounding = 2.0f;
    style.PopupRounding = 2.0f;

    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(5.0f, 3.0f);
    style.ItemSpacing = ImVec2(6.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.IndentSpacing = 12.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    // --- Colors ---
    ImVec4* colors = style.Colors;

    // Backgrounds
    colors[ImGuiCol_WindowBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);

    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Frame (inputs, checkboxes, etc.)
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);

    // Menubar
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);

    // Checkmark & Slider
    colors[ImGuiCol_CheckMark] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.70f, 1.00f, 1.00f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);

    // Headers (CollapsingHeader, TreeNode, Selectable)
    colors[ImGuiCol_Header] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);

    // Separator
    colors[ImGuiCol_Separator] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);

    // Resize grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.60f, 0.90f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.60f, 0.90f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.60f, 0.90f, 0.95f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);

    // Docking
    colors[ImGuiCol_DockingPreview] = ImVec4(0.35f, 0.60f, 0.90f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);

    // Plot
    colors[ImGuiCol_PlotLines] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.45f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.45f, 0.70f, 1.00f, 1.00f);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f); 
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.35f, 0.60f, 0.90f, 0.35f);

    // Drag & drop
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.35f, 0.60f, 0.90f, 0.90f);

    // Nav highlight
    colors[ImGuiCol_NavHighlight] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.55f);
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
    ImGui::DockBuilderDockWindow("Scene Configuration", dock_inspector);
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
    m_editorWindows.push_back(m_sceneConfig = new SceneConfig());

	D3D12Module* _d3d12 = app->getD3D12Module();

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
    /// THIS MUST BE EXECUTED AFTER RenderModule.h render functtion, if not F
    mainDockspace(&m_showMainDockspace);

    for (auto it = m_editorWindows.begin(); it != m_editorWindows.end(); ++it)
    {
        (*it)->render();
    }

    ImGui::EndFrame();
}

void EditorModule::render()
{

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
