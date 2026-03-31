#include "Globals.h"
#include "WindowAnimationStateMachine.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "AnimationStateMachineAsset.h"

#include <imgui.h>
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

namespace
{
    constexpr const char* NODE_EDITOR_ID = "AnimationStateMachineGraph";
    constexpr const char* NODE_EDITOR_SETTINGS_FILE = "AnimationStateMachineEditor.json";
}

WindowAnimationStateMachine::WindowAnimationStateMachine()
{
    setOpen(false);
    setSize(ImVec2(500.0f, 350.0f));
}

void WindowAnimationStateMachine::cleanUp()
{
    if (m_editorContext)
    {
        ed::SetCurrentEditor(nullptr);
        ed::DestroyEditor(m_editorContext);
        m_editorContext = nullptr;
    }

    m_asset.reset();
}

void WindowAnimationStateMachine::setTargetStateMachineUID(const MD5Hash& uid)
{
    if (m_targetStateMachineUID == uid)
    {
        return;
    }

    m_targetStateMachineUID = uid;
    m_asset.reset();
}

bool WindowAnimationStateMachine::ensureAssetLoaded()
{
    if (m_targetStateMachineUID == INVALID_ASSET_ID)
    {
        m_asset.reset();
        return false;
    }

    if (m_asset)
    {
        return true;
    }

    ModuleAssets* moduleAssets = app ? app->getModuleAssets() : nullptr;
    if (!moduleAssets)
    {
        return false;
    }

    m_asset = moduleAssets->load<AnimationStateMachineAsset>(m_targetStateMachineUID);
    return m_asset != nullptr;
}

bool WindowAnimationStateMachine::ensureEditorContext()
{
    if (m_editorContext)
    {
        return true;
    }

    ed::Config config;
    config.SettingsFile = NODE_EDITOR_SETTINGS_FILE;

    m_editorContext = ed::CreateEditor(&config);
    return m_editorContext != nullptr;
}


void WindowAnimationStateMachine::drawInternal()
{
    ImGui::TextUnformatted("Animation State Machine Editor");
    ImGui::Separator();

    ImGui::Text("Target UID: %s",
        m_targetStateMachineUID == INVALID_ASSET_ID ? "<none>" : m_targetStateMachineUID.c_str());

    if (m_targetStateMachineUID == INVALID_ASSET_ID)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("No state machine selected.");
    }
    else if (!ensureAssetLoaded())
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Could not load AnimationStateMachineAsset.");
    }
    else
    {
        ImGui::Spacing();
        ImGui::Text("Resource Name: %s", m_asset->getName().c_str());
        ImGui::Text("Default State: %s",
            m_asset->getDefaultStateName().empty() ? "<none>" : m_asset->getDefaultStateName().c_str());

        ImGui::Separator();

        ImGui::Text("Clips: %d", static_cast<int>(m_asset->getClips().size()));
        ImGui::Text("States: %d", static_cast<int>(m_asset->getStates().size()));
        ImGui::Text("Transitions: %d", static_cast<int>(m_asset->getTransitions().size()));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Graph editor context initialized in this commit.");

    if (!ensureEditorContext())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Could not create imgui-node-editor context.");
        return;
    }

    ed::SetCurrentEditor(m_editorContext);
    ed::Begin(NODE_EDITOR_ID, ImVec2(0.0f, 0.0f));

    ed::Suspend();
    ImGui::TextDisabled("Empty graph canvas.");
    ImGui::TextDisabled("Next commit: draw one node per state.");
    ed::Resume();

    ed::End();
    ed::SetCurrentEditor(nullptr);
}
