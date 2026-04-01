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

    constexpr int STATE_NODE_ID_BASE = 1000;
    constexpr int STATE_INPUT_PIN_ID_BASE = 100000;
    constexpr int STATE_OUTPUT_PIN_ID_BASE = 200000;
    constexpr int TRANSITION_LINK_ID_BASE = 300000;

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

    ed::LinkId getTransitionLinkId(int transitionIndex)
    {
        return ed::LinkId(TRANSITION_LINK_ID_BASE + transitionIndex);
    }

    int findStateIndexByName(const std::vector<AnimationStateMachineState>& states, const std::string& stateName)
    {
        for (int i = 0; i < static_cast<int>(states.size()); ++i)
        {
            if (states[i].name == stateName)
            {
                return i;
            }
        }

        return -1;
    }
}

WindowAnimationStateMachine::WindowAnimationStateMachine()
{
    setOpen(false);
    setSize(ImVec2(500.0f, 350.0f));
}

void WindowAnimationStateMachine::cleanUp()
{
    destroyEditorContext();

    m_asset.reset();
    m_needsInitialNodeLayout = true;
    m_focusContentNextFrame = false;
    m_isDirty = false;
}

void WindowAnimationStateMachine::setTargetStateMachineUID(const MD5Hash& uid)
{
    if (m_targetStateMachineUID == uid)
    {
        return;
    }

    destroyEditorContext();

    m_targetStateMachineUID = uid;
    m_asset.reset();
    m_isDirty = false;

    if (m_targetStateMachineUID == INVALID_ASSET_ID)
    {
        m_editorSettingsFile.clear();
    }
    else
    {
        m_editorSettingsFile = "AnimationStateMachineEditor_" + m_targetStateMachineUID + ".json";
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
    if (m_asset)
    {
        m_isDirty = false;
    }
    return m_asset != nullptr;
}

bool WindowAnimationStateMachine::ensureEditorContext()
{
    if (m_editorContext)
    {
        return true;
    }
    if (m_targetStateMachineUID == INVALID_ASSET_ID)
    {
        return false;
    }

    if (m_editorSettingsFile.empty())
    {
        return false;
    }

    ed::Config config;
    config.SettingsFile = m_editorSettingsFile.c_str();

    m_editorContext = ed::CreateEditor(&config);
    return m_editorContext != nullptr;
}

void WindowAnimationStateMachine::destroyEditorContext()
{
    if (!m_editorContext)
    {
        return;
    }

    ed::SetCurrentEditor(nullptr);
    ed::DestroyEditor(m_editorContext);
    m_editorContext = nullptr;
}

void WindowAnimationStateMachine::drawHeaderUi()
{
    ImGui::Text("Target UID: %s",
        m_targetStateMachineUID == INVALID_ASSET_ID ? "<none>" : m_targetStateMachineUID.c_str());

    ImGui::SameLine();

    if (m_targetStateMachineUID == INVALID_ASSET_ID)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("No state machine selected.");
        return;
    }

    if (!ensureAssetLoaded())
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Could not load AnimationStateMachineAsset.");
        return;
    }

    drawAssetSummaryUi();
}

void WindowAnimationStateMachine::drawAssetSummaryUi() const
{
    if (!m_asset)
    {
        return;
    }

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

void WindowAnimationStateMachine::drawGraphContent()
{
    if (!m_asset)
    {
        drawUnavailableGraphMessage("Graph unavailable without a loaded state machine.");
        return;
    }

    if (m_asset->getStates().empty())
    {
        drawUnavailableGraphMessage("State machine has no states.");
        return;
    }

    drawStateNodes();
    drawTransitionLinks();
}

void WindowAnimationStateMachine::drawUnavailableGraphMessage(const char* message)
{
    ed::Suspend();
    ImGui::TextDisabled("%s", message);
    ed::Resume();
}

void WindowAnimationStateMachine::drawStateNodes()
{
    if (!m_asset)
    {
        return;
    }

    const auto& states = m_asset->getStates();
    const std::string& defaultStateName = m_asset->getDefaultStateName();

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
}

void WindowAnimationStateMachine::drawTransitionLinks()
{
    if (!m_asset)
    {
        return;
    }

    const auto& states = m_asset->getStates();
    const auto& transitions = m_asset->getTransitions();

    for (int transitionIndex = 0; transitionIndex < static_cast<int>(transitions.size()); ++transitionIndex)
    {
        const AnimationStateMachineTransition& transition = transitions[transitionIndex];

        const int sourceStateIndex = findStateIndexByName(states, transition.sourceStateName);
        const int targetStateIndex = findStateIndexByName(states, transition.targetStateName);

        if (sourceStateIndex < 0 || targetStateIndex < 0)
        {
            continue;
        }

        const ed::LinkId linkId = getTransitionLinkId(transitionIndex);
        const ed::PinId sourcePinId = getStateOutputPinId(sourceStateIndex);
        const ed::PinId targetPinId = getStateInputPinId(targetStateIndex);

        ed::Link(linkId, sourcePinId, targetPinId);
    }
}

void WindowAnimationStateMachine::finalizeInitialLayout()
{
    if (m_needsInitialNodeLayout)
    {
        m_needsInitialNodeLayout = false;
    }
}

void WindowAnimationStateMachine::finalizeGraphFocus()
{
    if (m_focusContentNextFrame)
    {
        ed::NavigateToContent();
        m_focusContentNextFrame = false;
    }
}

bool WindowAnimationStateMachine::tryGetInputStateIndex(ax::NodeEditor::PinId pinId, int& outStateIndex) const
{
    outStateIndex = -1;

    if (!m_asset)
    {
        return false;
    }

    const auto& states = m_asset->getStates();
    for (int i = 0; i < static_cast<int>(states.size()); ++i)
    {
        if (pinId == getStateInputPinId(i))
        {
            outStateIndex = i;
            return true;
        }
    }

    return false;
}

bool WindowAnimationStateMachine::tryGetOutputStateIndex(ax::NodeEditor::PinId pinId, int& outStateIndex) const
{
    outStateIndex = -1;

    if (!m_asset)
    {
        return false;
    }

    const auto& states = m_asset->getStates();
    for (int i = 0; i < static_cast<int>(states.size()); ++i)
    {
        if (pinId == getStateOutputPinId(i))
        {
            outStateIndex = i;
            return true;
        }
    }

    return false;
}

bool WindowAnimationStateMachine::tryGetTransitionIndex(ax::NodeEditor::LinkId linkId, int& outTransitionIndex) const
{
    outTransitionIndex = -1;

    if (!m_asset)
    {
        return false;
    }

    const auto& transitions = m_asset->getTransitions();
    const int linkValue = static_cast<int>(linkId.Get());
    const int transitionIndex = linkValue - TRANSITION_LINK_ID_BASE;

    if (transitionIndex < 0 || transitionIndex >= static_cast<int>(transitions.size()))
    {
        return false;
    }

    outTransitionIndex = transitionIndex;
    return true;
}

bool WindowAnimationStateMachine::tryResolveTransitionEndpoints(ax::NodeEditor::PinId firstPinId, ax::NodeEditor::PinId secondPinId, int& outSourceStateIndex, int& outTargetStateIndex) const
{
    outSourceStateIndex = -1;
    outTargetStateIndex = -1;

    if (!firstPinId || !secondPinId)
    {
        return false;
    }

    const bool firstIsOutput = tryGetOutputStateIndex(firstPinId, outSourceStateIndex);
    const bool secondIsInput = tryGetInputStateIndex(secondPinId, outTargetStateIndex);

    if (firstIsOutput && secondIsInput)
    {
        return true;
    }

    const bool firstIsInput = tryGetInputStateIndex(firstPinId, outTargetStateIndex);
    const bool secondIsOutput = tryGetOutputStateIndex(secondPinId, outSourceStateIndex);

    if (firstIsInput && secondIsOutput)
    {
        return true;
    }

    outSourceStateIndex = -1;
    outTargetStateIndex = -1;
    return false;
}

bool WindowAnimationStateMachine::hasTransitionBetweenStates(const std::string& sourceStateName, const std::string& targetStateName) const
{
    if (!m_asset)
    {
        return false;
    }

    const auto& transitions = m_asset->getTransitions();
    for (const AnimationStateMachineTransition& transition : transitions)
    {
        if (transition.sourceStateName == sourceStateName &&
            transition.targetStateName == targetStateName)
        {
            return true;
        }
    }

    return false;
}

void WindowAnimationStateMachine::markDirty()
{
    m_isDirty = true;
}

void WindowAnimationStateMachine::sanitizeAssetAfterEdit()
{
    if (!m_asset)
    {
        return;
    }

    auto& clips = m_asset->getClipsMutable();
    auto& states = m_asset->getStatesMutable();
    auto& transitions = m_asset->getTransitionsMutable();
    std::string& defaultStateName = m_asset->getDefaultStateNameMutable();

    auto clipExists = [&](const std::string& clipName) -> bool
        {
            for (const AnimationStateMachineClip& clip : clips)
            {
                if (clip.name == clipName)
                {
                    return true;
                }
            }

            return false;
        };

    auto stateExists = [&](const std::string& stateName) -> bool
        {
            for (const AnimationStateMachineState& state : states)
            {
                if (state.name == stateName)
                {
                    return true;
                }
            }

            return false;
        };

    for (AnimationStateMachineState& state : states)
    {
        if (!clipExists(state.clipName))
        {
            state.clipName.clear();
        }

        if (state.speed < 0.0f)
        {
            state.speed = 0.0f;
        }
    }

    transitions.erase(
        std::remove_if(
            transitions.begin(),
            transitions.end(),
            [&](const AnimationStateMachineTransition& transition)
            {
                return !stateExists(transition.sourceStateName) ||
                    !stateExists(transition.targetStateName);
            }),
        transitions.end());

    for (AnimationStateMachineTransition& transition : transitions)
    {
        if (transition.blendTimeSeconds < 0.0f)
        {
            transition.blendTimeSeconds = 0.0f;
        }
    }

    if (!defaultStateName.empty() && !stateExists(defaultStateName))
    {
        defaultStateName.clear();
    }

    if (defaultStateName.empty() && !states.empty())
    {
        defaultStateName = states.front().name;
    }
}

bool WindowAnimationStateMachine::saveAsset()
{
    if (!m_asset)
    {
        return false;
    }

    ModuleAssets* moduleAssets = app ? app->getModuleAssets() : nullptr;
    if (!moduleAssets)
    {
        return false;
    }

    if (!moduleAssets->saveAnimationStateMachine(m_asset))
    {
        return false;
    }

    m_isDirty = false;
    return true;
}

void WindowAnimationStateMachine::drawInternal()
{
    drawHeaderUi();

    ImGui::Spacing();
    ImGui::Separator();

    if (!ensureEditorContext())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Could not create imgui-node-editor context.");
        return;
    }

    ed::SetCurrentEditor(m_editorContext);
    ed::Begin(NODE_EDITOR_ID, ImVec2(0.0f, 0.0f));

    drawGraphContent();
    finalizeInitialLayout();
    finalizeGraphFocus();

    ed::End();
    ed::SetCurrentEditor(nullptr);
}
