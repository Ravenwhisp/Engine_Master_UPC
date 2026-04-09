#include "Globals.h"
#include "ModuleEditor.h"

#include "ModuleD3D12.h"
#include "ModuleCamera.h"
#include <backends/imgui_impl_dx12.h>

#include "Scene.h"
#include "WindowSceneEditor.h"
#include "WindowHardware.h"
#include "WindowPerformance.h"
#include "EditorWindow.h"
#include "ImGuizmo.h"
#include "WindowLogger.h"
#include "ImGuiPass.h"
#include "WindowHierarchy.h"
#include "WindowInspector.h"
#include "WindowEditorSettings.h"
#include "WindowFileDialog.h"
#include "SceneConfig.h"
#include "WindowGame.h"
#include "WindowGameDebug.h"
#include "PrefabManager.h"
#include "ModuleRender.h"
#include "WindowAnimationStateMachine.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleGameView.h"
#include "Mouse.h"

#include <fstream>

using namespace std;

void ModuleEditor::mainMenuBar()
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

        if (ImGui::BeginMenu("Window"))
        {
            for (EditorWindow* window : m_editorWindows)
            {
                bool open = window->isOpen();
                if (ImGui::MenuItem(window->getWindowName(), nullptr, &open))
                {
                    window->setOpen(open);
                }
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void style()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
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

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.60f, 0.90f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.60f, 0.90f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.60f, 0.90f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.35f, 0.60f, 0.90f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.45f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.45f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.35f, 0.60f, 0.90f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.35f, 0.60f, 0.90f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.35f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.55f);
}

ModuleEditor::ModuleEditor() = default;

ModuleEditor::~ModuleEditor() = default;

void ModuleEditor::mainDockspace(bool* p_open)
{
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
    ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_None);

    if (m_firstFrame)
    {
        std::ifstream iniFile("imgui.ini");
        if (!iniFile.good())
        {
            setupDockLayout(dockspace_id);
        }
        style();
        m_firstFrame = false;
    }

    ImGui::End();
}


static const char* WINDOW_STATES_FILE = "editor_windows.ini";

void ModuleEditor::saveWindowStates()
{
    std::ofstream file(WINDOW_STATES_FILE);
    if (!file) return;

    for (EditorWindow* window : m_editorWindows)
    {
        file << window->getWindowName() << "=" << (window->isOpen() ? 1 : 0) << "\n";
    }
}

void ModuleEditor::loadWindowStates()
{
    std::ifstream file(WINDOW_STATES_FILE);
    if (!file) return;

    std::unordered_map<std::string, bool> states;
    std::string line;
    while (std::getline(file, line))
    {
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string name = line.substr(0, eq);
        const bool open = line.substr(eq + 1) == "1";
        states[name] = open;
    }

    for (EditorWindow* window : m_editorWindows)
    {
        auto it = states.find(window->getWindowName());
        if (it != states.end())
            window->setOpen(it->second);
    }
}

void ModuleEditor::setupDockLayout(ImGuiID dockspace_id)
{
    ImGui::DockBuilderRemoveNodeDockedWindows(dockspace_id);
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID dock_main = dockspace_id;
    ImGuiID dock_left;
    ImGuiID dock_inspector;
    ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.75f, &dock_left, &dock_inspector);

    ImGuiID dock_bottom;
    ImGuiID dock_top;
    ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_top);

    ImGuiID dock_hierarchy;
    ImGuiID dock_scene;
    ImGui::DockBuilderSplitNode(dock_top, ImGuiDir_Left, 0.25f, &dock_hierarchy, &dock_scene);

    ImGuiID dock_playmode_buttons;
    ImGui::DockBuilderSplitNode(dock_scene, ImGuiDir_Up, 0.1f, &dock_playmode_buttons, &dock_scene);

    ImGui::DockBuilderDockWindow("WindowInspector", dock_inspector);
    ImGui::DockBuilderDockWindow("Scene Configuration", dock_inspector);
    ImGui::DockBuilderDockWindow("Hierarchy", dock_hierarchy);
    ImGui::DockBuilderDockWindow("Editor Settings", dock_hierarchy);
    ImGui::DockBuilderDockWindow("Scene Editor", dock_scene);
    ImGui::DockBuilderDockWindow("Game", dock_scene);
    ImGui::DockBuilderDockWindow("FileDialog", dock_bottom);
    ImGui::DockBuilderDockWindow("Console", dock_bottom);
    ImGui::DockBuilderDockWindow("Hardware Info", dock_bottom);
    ImGui::DockBuilderDockWindow("Performance", dock_bottom);
    ImGui::DockBuilderDockWindow("Play Mode Buttons", dock_playmode_buttons);

    ImGui::DockBuilderFinish(dockspace_id);
}

bool ModuleEditor::init()
{
    m_moduleGameView = app->getModuleGameView();

    m_editorWindows.push_back(m_logger = new WindowLogger());
    m_editorWindows.push_back(m_hardwareWindow = new WindowHardware());
    m_editorWindows.push_back(m_performanceWindow = new WindowPerformance());
    m_editorWindows.push_back(m_editorSettings = new WindowEditorSettings());
    m_editorWindows.push_back(new WindowFileDialog());
    m_editorWindows.push_back(m_sceneConfig = new SceneConfig());
    

    m_sceneEditor = new WindowSceneEditor();
    m_editorWindows.push_back(m_sceneEditor);

    WindowHierarchy* hierarchy = new WindowHierarchy();
    WindowInspector* inspector = new WindowInspector();

    m_editorWindows.push_back(hierarchy);
    m_editorWindows.push_back(inspector);

    m_editorWindows.push_back(m_gameWindow = new WindowGame());

    m_editorWindows.push_back(m_windowAnimationStateMachine = new WindowAnimationStateMachine());

    m_viewGameDebug = std::make_unique<WindowGameDebug>();

    loadWindowStates();

    return true;
}

void ModuleEditor::update()
{
#ifdef GAME_RELEASE
    return;
#endif

    flushExitPrefabEdit();

    if (m_sceneEditor->isFocused())
    {
        handleKeyboardShortcuts();
    }
}

void ModuleEditor::render()
{
    ImGuizmo::BeginFrame();

#ifdef GAME_RELEASE
    if (m_moduleGameView->getShowDebugWindow() && m_viewGameDebug)
    {
        m_viewGameDebug->render();
    }

    ImGui::EndFrame();
    return;
#endif

    mainDockspace(&m_showMainDockspace);

    for (auto it = m_editorWindows.begin(); it != m_editorWindows.end(); ++it)
    {
        (*it)->draw();
    }

    if (m_moduleGameView->getShowDebugWindow() && m_viewGameDebug)
    {
        m_viewGameDebug->render();
    }

    ImGui::EndFrame();
}

bool ModuleEditor::cleanUp()
{
    app->getModuleD3D12()->getCommandQueue()->flush();

    saveWindowStates();

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
    m_gameWindow = nullptr;
    m_windowAnimationStateMachine = nullptr;

    return true;
}

ImVec2 ModuleEditor::getEventViewport() const
{
    WindowSceneEditor* sceneEditor = app->getModuleEditor()->getWindowSceneEditor();
    if (sceneEditor && sceneEditor->isFocused())
    {
        return ImVec2(sceneEditor->getViewportX(), sceneEditor->getViewportY());
    }

    WindowGame* gameWindow = app->getModuleEditor()->getWindowGame();
    if (gameWindow && gameWindow->isFocused())
    {
        return ImVec2(gameWindow->getViewportX(), gameWindow->getViewportY());
    }

    return ImVec2(-1, -1);
}

ImVec2 ModuleEditor::getEventViewportSize() const
{
    WindowSceneEditor* sceneEditor = app->getModuleEditor()->getWindowSceneEditor();
    if (sceneEditor && sceneEditor->isFocused())
    {
        return sceneEditor->getSize();
    }

    WindowGame* gameWindow = app->getModuleEditor()->getWindowGame();
    if (gameWindow && gameWindow->isFocused())
    {
        return gameWindow->getSize();
    }

    return ImVec2(-1, -1);
}

void ModuleEditor::setSceneTool(SCENE_TOOL newTool)
{
    if (currentSceneTool == newTool)
    {
        toggleGizmoMode();
        return;
    }

    currentSceneTool = newTool;
}

void ModuleEditor::setMode(SCENE_TOOL sceneTool, NAVIGATION_MODE navigationMode)
{
    if (previousSceneTool == NONE)
    {
        previousSceneTool = currentSceneTool;
        currentSceneTool = sceneTool;
        currentNavigationMode = navigationMode;
    }
}

void ModuleEditor::resetMode()
{
    if (previousSceneTool != NONE)
    {
        currentSceneTool = previousSceneTool;
        previousSceneTool = NONE;
    }

    currentNavigationMode = PAN;
}

void ModuleEditor::handleKeyboardShortcuts()
{
    Keyboard::State keyboardState = Keyboard::Get().GetState();
    Mouse::State mouseState = Mouse::Get().GetState();

    DirectX::Mouse::ButtonStateTracker buttonStateTracker;
    buttonStateTracker.Update(mouseState);

    if (mouseState.rightButton)
    {
        setMode(NAVIGATION, FREE_LOOK);
    }
    else if (mouseState.leftButton && (keyboardState.LeftAlt || keyboardState.RightAlt))
    {
        setMode(NAVIGATION, ORBIT);
    }
    else if (mouseState.middleButton)
    {
        setMode(NAVIGATION, PAN);
    }
    else if (!mouseState.leftButton)
    {
        resetMode();
        handleQWERTYCases(keyboardState);
    }
}

void ModuleEditor::handleQWERTYCases(Keyboard::State keyboardState)
{
    static Keyboard::KeyboardStateTracker keyTracker;
    keyTracker.Update(keyboardState);

    if (keyTracker.pressed.W)
    {
        setSceneTool(MOVE);
    }

    if (keyTracker.pressed.E)
    {
        setSceneTool(ROTATE);
    }

    if (keyTracker.pressed.R)
    {
        setSceneTool(SCALE);
    }

    if (keyTracker.pressed.T)
    {
        setSceneTool(RECT);
    }

    if (keyTracker.pressed.Y)
    {
        setSceneTool(TRANSFORM);
    }

    if (keyTracker.pressed.Q)
    {
        currentSceneTool = NAVIGATION;
        currentNavigationMode = PAN;
    }
}

void ModuleEditor::enterPrefabEdit(const std::filesystem::path& sourcePath)
{
    app->getModuleD3D12()->getCommandQueue()->flush();

    if (m_prefabSession.m_active)
    {
        if (m_prefabSession.m_isolatedScene && m_prefabSession.m_isolatedScene != app->getModuleScene()->getScene())
        {
            delete m_prefabSession.m_isolatedScene;
        }
        m_prefabSession.clear();
    }

    Scene* isolatedScene = new Scene();
    m_prefabSession.m_isolatedScene = isolatedScene;

    GameObject* loaded = PrefabManager::instantiatePrefab(sourcePath, isolatedScene);

    if (!loaded)
    {
        delete isolatedScene;
        m_prefabSession.clear();
        return;
    }

    m_prefabSession.m_sourcePath = sourcePath;
    m_prefabSession.m_rootObject = loaded;
    m_prefabSession.m_active = true;
    m_prefabSession.m_editingInMainScene = false;
    m_selectedGameObject = loaded;
}

void ModuleEditor::exitPrefabEdit()
{
    if (!m_prefabSession.m_active)
    {
        return;
    }

    m_pendingExitPrefab = true;
}

void ModuleEditor::flushExitPrefabEdit()
{
    if (!m_pendingExitPrefab)
    {
        return;
    }

    m_pendingExitPrefab = false;
    app->getModuleD3D12()->getCommandQueue()->flush();
    m_selectedGameObject = nullptr;

    if (m_prefabSession.m_isolatedScene && m_prefabSession.m_isolatedScene != app->getModuleScene()->getScene())
    {
        delete m_prefabSession.m_isolatedScene;
        m_prefabSession.m_isolatedScene = nullptr;
    }

    m_prefabSession.clear();
}
