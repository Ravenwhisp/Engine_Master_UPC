#include "Globals.h"
#include "WindowAnimationStateMachine.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "AnimationStateMachineAsset.h"

#include <filesystem>
#include <system_error>
#include <imgui.h>
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

namespace
{
    constexpr const char* NODE_EDITOR_ID = "AnimationStateMachineGraph";
    constexpr const char* NODE_EDITOR_SETTINGS_FOLDER = "EditorSettings/AnimationStateMachines";

    constexpr int STATE_NODE_ID_BASE = 1000;
    constexpr int STATE_INPUT_PIN_ID_BASE = 100000;
    constexpr int STATE_OUTPUT_PIN_ID_BASE = 200000;

    ed::NodeId getStateNodeId(int stateIndex)
    {
        return ed::NodeId(STATE_NODE_ID_BASE + stateIndex);
    }

    ed::PinId getStateInputPinId(int stateIndex)
    {
        return ed::PinId(STATE_INPUT_PIN_ID_BASE + stateIndex);
    }

    ed::PinId getStateOutputPinId(int stateIndex)
    {
        return ed::PinId(STATE_OUTPUT_PIN_ID_BASE + stateIndex);
    }
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
    m_needsInitialNodeLayout = true;
}

void WindowAnimationStateMachine::setTargetStateMachineUID(const MD5Hash& uid)
{
    if (m_targetStateMachineUID == uid)
    {
        return;
    }

    if (m_editorContext)
    {
        ed::SetCurrentEditor(nullptr);
        ed::DestroyEditor(m_editorContext);
        m_editorContext = nullptr;
    }

    m_targetStateMachineUID = uid;
    m_asset.reset();

    if (m_targetStateMachineUID == INVALID_ASSET_ID)
    {
        m_editorSettingsFile.clear();
    }
    else
    {
        const std::filesystem::path settingsDir = NODE_EDITOR_SETTINGS_FOLDER;

        m_editorSettingsFile =
            (settingsDir / ("AnimationStateMachineEditor_" + m_targetStateMachineUID + ".json")).string();
    }

    m_needsInitialNodeLayout = true;
    m_focusContentNextFrame = true;
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

    if (!m_editorSettingsFile.empty())
    {
        const std::filesystem::path settingsPath(m_editorSettingsFile);

        std::error_code ec;
        std::filesystem::create_directories(settingsPath.parent_path(), ec);

        if (ec)
        {
            DEBUG_WARN("[WindowAnimationStateMachine] Could not create settings folder '%s' (%s).",
                settingsPath.parent_path().string().c_str(),
                ec.message().c_str());
        }
        else
        {
            config.SettingsFile = m_editorSettingsFile.c_str();
        }
    }

    m_editorContext = ed::CreateEditor(&config);
    return m_editorContext != nullptr;
}

void WindowAnimationStateMachine::drawInternal()
{
    ImGui::Text("Target UID: %s",
        m_targetStateMachineUID == INVALID_ASSET_ID ? "<none>" : m_targetStateMachineUID.c_str());

    ImGui::SameLine();

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
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::Text("Resource Name: %s", m_asset->getName().c_str());
        ImGui::SameLine();
        ImGui::Text("Default State: %s",
            m_asset->getDefaultStateName().empty() ? "<none>" : m_asset->getDefaultStateName().c_str());

        ImGui::Separator();

        ImGui::Text("Clips: %d", static_cast<int>(m_asset->getClips().size()));
        ImGui::SameLine();
        ImGui::Text("States: %d", static_cast<int>(m_asset->getStates().size()));
        ImGui::SameLine();
        ImGui::Text("Transitions: %d", static_cast<int>(m_asset->getTransitions().size()));
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (!ensureEditorContext())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Could not create imgui-node-editor context.");
        return;
    }

    ed::SetCurrentEditor(m_editorContext);
    ed::Begin(NODE_EDITOR_ID, ImVec2(0.0f, 0.0f));

    if (!m_asset)
    {
        ed::Suspend();
        ImGui::TextDisabled("Graph unavailable without a loaded state machine.");
        ed::Resume();
    }
    else
    {
        const auto& states = m_asset->getStates();
        const std::string& defaultStateName = m_asset->getDefaultStateName();

        if (states.empty())
        {
            ed::Suspend();
            ImGui::TextDisabled("State machine has no states.");
            ed::Resume();
        }
        else
        {
            for (int i = 0; i < static_cast<int>(states.size()); ++i)
            {
                const AnimationStateMachineState& state = states[i];
                const bool isDefaultState = (state.name == defaultStateName);

                const ed::NodeId nodeId = getStateNodeId(i);
                const ed::PinId inputPinId = getStateInputPinId(i);
                const ed::PinId outputPinId = getStateOutputPinId(i);

                ed::BeginNode(nodeId);

                if (isDefaultState)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", state.name.c_str());
                }
                else
                {
                    ImGui::TextUnformatted(state.name.c_str());
                }

                ImGui::Separator();
                ImGui::Text("Clip: %s", state.clipName.empty() ? "<none>" : state.clipName.c_str());

                if (isDefaultState)
                {
                    ImGui::TextUnformatted("Default");
                }
                else
                {
                    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
                }

                ImGui::Spacing();

                ed::BeginPin(inputPinId, ed::PinKind::Input);
                ImGui::TextUnformatted("In");
                ed::EndPin();

                ImGui::SameLine();
                ImGui::Dummy(ImVec2(60.0f, 0.0f));
                ImGui::SameLine();

                ed::BeginPin(outputPinId, ed::PinKind::Output);
                ImGui::TextUnformatted("Out");
                ed::EndPin();

                ed::EndNode();

                if (m_needsInitialNodeLayout)
                {
                    const float x = 40.0f + static_cast<float>(i % 3) * 260.0f;
                    const float y = 40.0f + static_cast<float>(i / 3) * 170.0f;
                    ed::SetNodePosition(nodeId, ImVec2(x, y));
                }
            }

            if (m_needsInitialNodeLayout)
            {
                m_needsInitialNodeLayout = false;
            }
        }
    }

    if (m_needsInitialNodeLayout)
    {
        m_needsInitialNodeLayout = false;
    }

    if (m_focusContentNextFrame)
    {
        ed::NavigateToContent();
        m_focusContentNextFrame = false;
    }

    ed::End();
    ed::SetCurrentEditor(nullptr);
}
