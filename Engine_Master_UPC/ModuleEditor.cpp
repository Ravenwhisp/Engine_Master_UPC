#include "Globals.h"
#include "ModuleEditor.h"

#include "ModuleD3D12.h"
#include "ModuleCamera.h"
#include <backends/imgui_impl_dx12.h>

#include "WindowSceneEditor.h"
#include "WindowHardware.h"
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

static const char* WINDOW_STATES_FILE = "editor_windows.ini";

static void applyImGuiStyle()
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


EditorWindow* ModuleEditor::openWindow(const std::string& typeKey)
{
    auto it = m_windowFactories.find(typeKey);
    if (it == m_windowFactories.end())
    {
        return nullptr;
    }

    EditorWindow* window = it->second();
    window->setInstanceId(m_nextInstanceId++);
    m_editorWindows.push_back(window);
    return window;
}


bool ModuleEditor::init()
{
    m_moduleGameView = app->getModuleGameView();

    // ---- Register all spawnable window types ----
    registerWindowType<WindowLogger>("Console");
    registerWindowType<WindowHardware>("Hardware Info");
    registerWindowType<WindowEditorSettings>("Editor Settings");
    registerWindowType<WindowFileDialog>("FileDialog");
    registerWindowType<SceneConfig>("Scene Configuration");
    registerWindowType<WindowSceneEditor>("Scene Editor");
    registerWindowType<WindowHierarchy>("Hierarchy");
    registerWindowType<WindowInspector>("WindowInspector");
    registerWindowType<WindowGame>("Game");
    registerWindowType<WindowAnimationStateMachine>("Animation State Machine");

    // ---- Spawn the default set of windows (one each) ----
    openWindow("Console");
    openWindow("Hardware Info");
    openWindow("Performance");
    openWindow("Editor Settings");
    openWindow("FileDialog");
    openWindow("Scene Configuration");
    openWindow("Scene Editor");
    openWindow("Hierarchy");
    openWindow("WindowInspector");
    openWindow("Game");
    openWindow("Animation State Machine");

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

    if (WindowSceneEditor* sceneEditor = findWindow<WindowSceneEditor>())
    {
        if (sceneEditor->isFocused())
        {
            handleKeyboardShortcuts();
        }
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

    for (EditorWindow* window : m_editorWindows)
    {
        window->draw();
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

    for (EditorWindow* window : m_editorWindows)
    {
        window->cleanUp();
    }

    for (EditorWindow* window : m_editorWindows)
    {
        delete window;
    }

    m_editorWindows.clear();

    return true;
}

WindowSceneEditor* ModuleEditor::getWindowSceneEditor() const
{
    return findWindow<WindowSceneEditor>();
}

WindowGame* ModuleEditor::getWindowGame() const
{
    return findWindow<WindowGame>();
}

WindowAnimationStateMachine* ModuleEditor::getWindowAnimationStateMachine() const
{
    return findWindow<WindowAnimationStateMachine>();
}

ImVec2 ModuleEditor::getEventViewport() const
{
    if (WindowSceneEditor* s = findWindow<WindowSceneEditor>())
    {
        if (s->isFocused())
        {
            return ImVec2(s->getViewportX(), s->getViewportY());
        }
    }

    if (WindowGame* g = findWindow<WindowGame>())
    {
        if (g->isFocused())
        {
            return ImVec2(g->getViewportX(), g->getViewportY());
        }
    }

    return ImVec2(-1, -1);
}

ImVec2 ModuleEditor::getEventViewportSize() const
{
    if (WindowSceneEditor* s = findWindow<WindowSceneEditor>())
    {
        if (s->isFocused())
        {
            return s->getSize();
        }
    }

    if (WindowGame* g = findWindow<WindowGame>())
    {
        if (g->isFocused())
        {
            return g->getSize();
        }
    }

    return ImVec2(-1, -1);
}


void ModuleEditor::mainMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

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
        // Toggle visibility of existing windows.
        for (EditorWindow* window : m_editorWindows)
        {
            bool open = window->isOpen();
            // Show "Inspector (2)" style labels when multiple instances exist.
            std::string label = window->getWindowName();
            if (window->getInstanceId() > 1)
            {
                label += " (" + std::to_string(window->getInstanceId()) + ")";
            }

            if (ImGui::MenuItem(label.c_str(), nullptr, &open))
            {
                window->setOpen(open);
            }
        }

        // ---- New Window submenu — spawn additional instances ----
        if (!m_windowFactories.empty())
        {
            ImGui::Separator();
            if (ImGui::BeginMenu("New Window"))
            {
                for (const auto& [key, factory] : m_windowFactories)
                {
                    if (ImGui::MenuItem(key.c_str()))
                    {
                        openWindow(key);
                    }
                }
                ImGui::EndMenu();
            }
        }

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}


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
        applyImGuiStyle();
        m_firstFrame = false;
    }

    ImGui::End();
}

void ModuleEditor::setupDockLayout(ImGuiID dockspace_id)
{
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

    ImGuiID dock_playmode_buttons;
    ImGui::DockBuilderSplitNode(dock_scene, ImGuiDir_Up, 0.1f, &dock_playmode_buttons, &dock_scene);

    // Helper: dock the first window whose display name matches the given type key.
    auto dockFirstOfType = [&](const char* typeKey, ImGuiID node)
        {
            for (EditorWindow* w : m_editorWindows)
            {
                if (strcmp(w->getWindowName(), typeKey) == 0)
                {
                    ImGui::DockBuilderDockWindow(w->getImGuiId(), node);
                    break;
                }
            }
        };

    dockFirstOfType("WindowInspector", dock_inspector);
    dockFirstOfType("Scene Configuration", dock_inspector);
    dockFirstOfType("Hierarchy", dock_hierarchy);
    dockFirstOfType("Editor Settings", dock_hierarchy);
    dockFirstOfType("Scene Editor", dock_scene);
    dockFirstOfType("Game", dock_scene);
    dockFirstOfType("FileDialog", dock_bottom);
    dockFirstOfType("Console", dock_bottom);
    dockFirstOfType("Hardware Info", dock_bottom);
    dockFirstOfType("Performance", dock_bottom);
    dockFirstOfType("Play Mode Buttons", dock_playmode_buttons);

    ImGui::DockBuilderFinish(dockspace_id);
}


void ModuleEditor::saveWindowStates()
{
    std::ofstream file(WINDOW_STATES_FILE);
    if (!file)
    {
        return;
    }

    for (EditorWindow* window : m_editorWindows)
    {
        file << window->getWindowName() << "|"
            << window->getInstanceId() << "|"
            << (window->isOpen() ? 1 : 0) << "\n";
    }
}

void ModuleEditor::loadWindowStates()
{
    std::ifstream file(WINDOW_STATES_FILE);
    if (!file)
    {
        return;
    }

    // Build a map from instanceId to open-state for each type key.
    // e.g. savedStates["WindowInspector"][2] = false
    std::unordered_map<std::string,
        std::unordered_map<int, bool>> savedStates;

    int maxInstanceId = 0;

    std::string line;
    while (std::getline(file, line))
    {
        const size_t sep1 = line.find('|');
        if (sep1 == std::string::npos) continue;

        const size_t sep2 = line.find('|', sep1 + 1);
        if (sep2 == std::string::npos) continue;

        const std::string typeKey = line.substr(0, sep1);
        const int         instanceId = std::stoi(line.substr(sep1 + 1, sep2 - sep1 - 1));
        const bool        isOpen = (line.substr(sep2 + 1) == "1");

        savedStates[typeKey][instanceId] = isOpen;
        maxInstanceId = std::max(maxInstanceId, instanceId);
    }

    // Advance the counter so new windows never reuse a saved ID.
    if (maxInstanceId >= m_nextInstanceId)
    {
        m_nextInstanceId = maxInstanceId + 1;
    }

    for (const auto& [typeKey, instances] : savedStates)
    {
        for (const auto& [instanceId, isOpen] : instances)
        {
            // Check whether this instance already exists.
            EditorWindow* target = nullptr;
            for (EditorWindow* w : m_editorWindows)
            {
                if (strcmp(w->getWindowName(), typeKey.c_str()) == 0 &&
                    w->getInstanceId() == instanceId)
                {
                    target = w;
                    break;
                }
            }

            if (!target)
            {
                auto it = m_windowFactories.find(typeKey);
                if (it != m_windowFactories.end())
                {
                    target = it->second();           // construct
                    target->setInstanceId(instanceId); // restore original ID
                    m_editorWindows.push_back(target);
                }
            }

            if (target)
            {
                target->setOpen(isOpen);
            }
        }
    }
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
    Mouse::State    mouseState = Mouse::Get().GetState();

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

    if (keyTracker.pressed.W) setSceneTool(MOVE);
    if (keyTracker.pressed.E) setSceneTool(ROTATE);
    if (keyTracker.pressed.R) setSceneTool(SCALE);
    if (keyTracker.pressed.T) setSceneTool(RECT);
    if (keyTracker.pressed.Y) setSceneTool(TRANSFORM);

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
        m_prefabSession.clear();
    }

    m_prefabSession.m_isolatedScene = app->getModuleScene()->getScene();

    GameObject* loaded = PrefabManager::instantiatePrefab(sourcePath, m_prefabSession.m_isolatedScene);

    if (!loaded)
    {
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
    m_prefabSession.clear();
}