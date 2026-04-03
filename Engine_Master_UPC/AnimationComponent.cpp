#include "Globals.h"
#include "AnimationComponent.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleTime.h"
#include "ModuleEditor.h"
#include "WindowAnimationStateMachine.h"

#include "AnimationAsset.h"
#include "AnimationStateMachineAsset.h"
#include "GameObject.h"
#include "Transform.h"

#include <imgui.h>
#include <cstring>
#include <algorithm>

namespace
{
    void DebugLine(const Vector3& from, const Vector3& to, const Vector3& color, bool depthEnabled = false)
    {
        const float p0[3] = { from.x, from.y, from.z };
        const float p1[3] = { to.x, to.y, to.z };
        const float c[3] = { color.x, color.y, color.z };

        dd::line(p0, p1, c, 0, depthEnabled);
    }

    bool InputTextString(const char* label, std::string& value)
    {
        char buffer[256];
        std::strncpy(buffer, value.c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        if (ImGui::InputText(label, buffer, sizeof(buffer)))
        {
            value = buffer;
            return true;
        }

        return false;
    }
}

AnimationComponent::AnimationComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::ANIMATION, owner)
    , m_stateMachineUIDInput(m_stateMachineUID)
{
}

std::unique_ptr<Component> AnimationComponent::clone(GameObject* newOwner) const
{
    auto cloned = std::make_unique<AnimationComponent>(m_uuid, newOwner);

    cloned->m_stateMachineUID = m_stateMachineUID;
    cloned->m_playOnStart = m_playOnStart;
    cloned->m_applyScale = m_applyScale;
    cloned->m_forceWorldAfterApply = m_forceWorldAfterApply;
    cloned->m_debugDrawHierarchy = m_debugDrawHierarchy;
    cloned->m_stateMachineUIDInput = m_stateMachineUIDInput;

    cloned->setActive(isActive());
    return cloned;
}

bool AnimationComponent::init()
{
    ensureStateMachineLoaded();
    startStateMachineIfNeeded();
    return true;

}

bool AnimationComponent::cleanUp()
{
    resetRuntime();
    m_stateMachineAsset.reset();
    return true;
}

void AnimationComponent::update()
{
    GameObject* owner = getOwner();
    if (!owner)
        return;

    Transform* ownerTransform = owner->GetTransform();
    if (!ownerTransform)
        return;

    if (!app)
        return;

    ModuleTime* moduleTime = app->getModuleTime();
    if (!moduleTime)
        return;

    if (!ensureStateMachineLoaded())
        return;

    startStateMachineIfNeeded();

    if (m_activeStateName.empty())
        return;

    

    const float deltaTimeSeconds = moduleTime->deltaTime();
    m_controller.Update(deltaTimeSeconds);

    if (m_currentTransitionTime > 0.0f && m_previousPlayback)
    {
        m_currentFadeTime = std::min(m_currentFadeTime + deltaTimeSeconds, m_currentTransitionTime);
    }

    updateFadingPlaybackRecursive(m_previousPlayback.get(), deltaTimeSeconds);

    if (m_currentTransitionTime > 0.0f &&
        m_previousPlayback &&
        m_currentFadeTime >= m_currentTransitionTime)
    {
        m_previousPlayback.reset();
        m_currentFadeTime = 0.0f;
        m_currentTransitionTime = 0.0f;
    }

    applyRecursive(owner);

    if (m_forceWorldAfterApply)
    {
        forceWorldRecursive(owner);
    }

    if (m_debugDrawHierarchy)
    {
        debugDrawRecursive(owner);
    }
}

bool AnimationComponent::activateState(const std::string& stateName, bool autoPlay, float transitionTimeSeconds)
{
    if (!ensureStateMachineLoaded())
        return false;

    const AnimationStateMachineState* state = findStateByName(stateName);
    if (!state)
    {
        DEBUG_WARN("[AnimationComponent] State '%s' not found.", stateName.c_str());
        return false;
    }

    const AnimationStateMachineClip* clip = findClipByName(state->clipName);
    if (!clip)
    {
        DEBUG_WARN("[AnimationComponent] Clip '%s' not found for state '%s'.", state->clipName.c_str(), stateName.c_str());
        return false;
    }

    if (clip->animationUID == INVALID_ASSET_ID)
    {
        DEBUG_WARN("[AnimationComponent] Clip '%s' has invalid animation UID.", clip->name.c_str());
        return false;
    }

    ModuleAssets* moduleAssets = app ? app->getModuleAssets() : nullptr;
    if (!moduleAssets)
        return false;

    std::shared_ptr<AnimationAsset> animation = moduleAssets->load<AnimationAsset>(clip->animationUID);
    if (!animation)
    {
        DEBUG_WARN("[AnimationComponent] Could not load clip animation '%s'.", clip->animationUID.c_str());
        return false;
    }

    if (transitionTimeSeconds > 0.0f)
    {
        pushCurrentPlaybackToHistory(transitionTimeSeconds);
    }
    else
    {
        m_previousPlayback.reset();
        m_currentFadeTime = 0.0f;
        m_currentTransitionTime = 0.0f;
    }

    m_currentAnimationAsset = animation;
    m_activeStateName = state->name;

    m_controller.Stop();
    m_controller.SetAnimation(m_currentAnimationAsset);
    m_controller.SetLoop(clip->loop);
    m_controller.SetSpeed(state->speed);

    if (autoPlay)
    {
        m_controller.Play(clip->loop);
    }

    return true;
}

bool AnimationComponent::previewState(const std::string& stateName)
{
    if (!ensureStateMachineLoaded())
        return false;

    const bool ok = activateState(stateName, true, 0.0f);
    if (ok)
    {
        m_hasStartedPlayback = true;
    }

    return ok;
}

void AnimationComponent::applyRecursive(GameObject* go)
{
    if (!go)
        return;

    Transform* transform = go->GetTransform();
    if (!transform)
        return;

    AnimationSample sample;
    if (samplePlaybackRecursive(go->GetName(), sample))
    {
        if (sample.hasPosition)
            transform->setPosition(sample.position);

        if (sample.hasRotation)
            transform->setRotation(sample.rotation);

        if (m_applyScale && sample.hasScale)
            transform->setScale(sample.scale);
    }

    const auto& children = transform->getAllChildren();
    for (GameObject* child : children)
    {
        if (!child)
            continue;

        applyRecursive(child);
    }
}

void AnimationComponent::forceWorldRecursive(GameObject* go)
{
    if (!go)
        return;

    Transform* transform = go->GetTransform();
    if (!transform)
        return;

    transform->getGlobalMatrix();

    const auto& children = transform->getAllChildren();
    for (GameObject* child : children)
    {
        if (!child)
            continue;

        forceWorldRecursive(child);
    }
}

void AnimationComponent::pushCurrentPlaybackToHistory(float transitionTimeSeconds)
{
    if (!m_currentAnimationAsset || m_activeStateName.empty())
    {
        m_previousPlayback.reset();
        m_currentFadeTime = 0.0f;
        m_currentTransitionTime = 0.0f;
        return;
    }

    auto oldHead = std::make_unique<FadingPlayback>();
    oldHead->stateName = m_activeStateName;
    oldHead->animationAsset = m_currentAnimationAsset;
    oldHead->controller = m_controller;
    oldHead->fadeTime = m_currentFadeTime;
    oldHead->transitionTime = m_currentTransitionTime;
    oldHead->next = std::move(m_previousPlayback);

    m_previousPlayback = std::move(oldHead);
    m_currentFadeTime = 0.0f;
    m_currentTransitionTime = std::max(0.0f, transitionTimeSeconds);
}

void AnimationComponent::updateFadingPlaybackRecursive(FadingPlayback* playback, float deltaTimeSeconds)
{
    if (!playback)
        return;

    playback->controller.Update(deltaTimeSeconds);

    if (playback->transitionTime > 0.0f && playback->next)
    {
        playback->fadeTime = std::min(playback->fadeTime + deltaTimeSeconds, playback->transitionTime);
    }

    updateFadingPlaybackRecursive(playback->next.get(), deltaTimeSeconds);

    if (playback->transitionTime > 0.0f &&
        playback->next &&
        playback->fadeTime >= playback->transitionTime)
    {
        playback->next.reset();
        playback->fadeTime = 0.0f;
        playback->transitionTime = 0.0f;
    }
}

bool AnimationComponent::samplePlaybackRecursive(const std::string& channelName, AnimationSample& outSample) const
{
    AnimationSample currentSample;
    const bool hasCurrent = m_controller.GetTransform(channelName, currentSample);

    if (!m_previousPlayback)
    {
        if (hasCurrent)
        {
            outSample = currentSample;
            return true;
        }
        return false;
    }

    AnimationSample previousSample;
    const bool hasPrevious = samplePlaybackNodeRecursive(m_previousPlayback.get(), channelName, previousSample);

    if (!hasCurrent)
    {
        if (hasPrevious)
            outSample = previousSample;
        return hasPrevious;
    }

    if (!hasPrevious || m_currentTransitionTime <= 0.0f)
    {
        outSample = currentSample;
        return true;
    }

    const float weight = std::clamp(m_currentFadeTime / m_currentTransitionTime, 0.0f, 1.0f);
    blendSamples(previousSample, currentSample, weight, outSample);
    return true;
}

bool AnimationComponent::samplePlaybackNodeRecursive(const FadingPlayback* playback,
    const std::string& channelName,
    AnimationSample& outSample) const
{
    if (!playback)
        return false;

    AnimationSample currentSample;
    const bool hasCurrent = playback->controller.GetTransform(channelName, currentSample);

    if (!playback->next)
    {
        if (hasCurrent)
        {
            outSample = currentSample;
            return true;
        }
        return false;
    }

    AnimationSample previousSample;
    const bool hasPrevious = samplePlaybackNodeRecursive(playback->next.get(), channelName, previousSample);

    if (!hasCurrent)
    {
        if (hasPrevious)
            outSample = previousSample;
        return hasPrevious;
    }

    if (!hasPrevious || playback->transitionTime <= 0.0f)
    {
        outSample = currentSample;
        return true;
    }

    const float weight = std::clamp(playback->fadeTime / playback->transitionTime, 0.0f, 1.0f);
    blendSamples(previousSample, currentSample, weight, outSample);
    return true;
}

void AnimationComponent::blendSamples(const AnimationSample& fromSample,
    const AnimationSample& toSample,
    float weight,
    AnimationSample& outSample) const
{
    outSample = AnimationSample{};
    weight = std::clamp(weight, 0.0f, 1.0f);

    if (fromSample.hasPosition && toSample.hasPosition)
    {
        outSample.position = Vector3::Lerp(fromSample.position, toSample.position, weight);
        outSample.hasPosition = true;
    }
    else if (toSample.hasPosition)
    {
        outSample.position = toSample.position;
        outSample.hasPosition = true;
    }
    else if (fromSample.hasPosition)
    {
        outSample.position = fromSample.position;
        outSample.hasPosition = true;
    }

    if (fromSample.hasRotation && toSample.hasRotation)
    {
        outSample.rotation = Quaternion::Slerp(fromSample.rotation, toSample.rotation, weight);
        outSample.hasRotation = true;
    }
    else if (toSample.hasRotation)
    {
        outSample.rotation = toSample.rotation;
        outSample.hasRotation = true;
    }
    else if (fromSample.hasRotation)
    {
        outSample.rotation = fromSample.rotation;
        outSample.hasRotation = true;
    }

    if (fromSample.hasScale && toSample.hasScale)
    {
        outSample.scale = Vector3::Lerp(fromSample.scale, toSample.scale, weight);
        outSample.hasScale = true;
    }
    else if (toSample.hasScale)
    {
        outSample.scale = toSample.scale;
        outSample.hasScale = true;
    }
    else if (fromSample.hasScale)
    {
        outSample.scale = fromSample.scale;
        outSample.hasScale = true;
    }
}

void AnimationComponent::drawStateMachineResourceUi()
{
    if (!ensureStateMachineLoaded())
        return;

    if (!ImGui::CollapsingHeader("State Machine Resource", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    if (InputTextString("Resource Name", m_stateMachineAsset->getNameMutable()))
    {
        m_stateMachineDirty = true;
    }

    drawClipsUi();
    drawStatesUi();
    drawTransitionsUi();

    sanitizeStateMachineAfterEdit();
}

void AnimationComponent::drawClipsUi()
{
    if (!m_stateMachineAsset)
        return;

    auto& clips = m_stateMachineAsset->getClipsMutable();

    if (!ImGui::CollapsingHeader("Clips", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    for (size_t i = 0; i < clips.size(); ++i)
    {
        AnimationStateMachineClip& clip = clips[i];
        ImGui::PushID(static_cast<int>(i));

        if (ImGui::TreeNode("Clip", "Clip %zu", i))
        {
            if (InputTextString("Name", clip.name))
            {
                m_stateMachineDirty = true;
            }

            if (InputTextString("Animation UID", clip.animationUID))
            {
                m_stateMachineDirty = true;
            }

            if (ImGui::Checkbox("Loop", &clip.loop))
            {
                m_stateMachineDirty = true;
            }

            if (ImGui::Button("Delete Clip"))
            {
                clips.erase(clips.begin() + static_cast<std::ptrdiff_t>(i));
                ImGui::TreePop();
                ImGui::PopID();
                sanitizeStateMachineAfterEdit();
                m_stateMachineDirty = true;
                return;
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    if (ImGui::Button("Add Clip"))
    {
        AnimationStateMachineClip clip;
        clip.name = "NewClip";
        clip.animationUID = INVALID_ASSET_ID;
        clip.loop = true;
        clips.push_back(std::move(clip));
        m_stateMachineDirty = true;
        sanitizeStateMachineAfterEdit();
    }
}
void AnimationComponent::drawStatesUi()
{
    if (!m_stateMachineAsset)
        return;

    auto& states = m_stateMachineAsset->getStatesMutable();
    std::string& defaultState = m_stateMachineAsset->getDefaultStateNameMutable();

    if (!ImGui::CollapsingHeader("States", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    std::string oldDefaultState = defaultState;
    drawStateCombo("Default State", defaultState);
    if (defaultState != oldDefaultState)
    {
        m_stateMachineDirty = true;
    }

    for (size_t i = 0; i < states.size(); ++i)
    {
        AnimationStateMachineState& state = states[i];
        ImGui::PushID(static_cast<int>(i));

        if (ImGui::TreeNode("State", "State %zu", i))
        {
            if (InputTextString("Name", state.name))
            {
                m_stateMachineDirty = true;
            }

            std::string oldClipName = state.clipName;
            drawClipCombo("Clip", state.clipName);
            if (state.clipName != oldClipName)
            {
                m_stateMachineDirty = true;
            }

            if (ImGui::DragFloat("Speed", &state.speed, 0.05f, 0.0f, 10.0f))
            {
                m_stateMachineDirty = true;
            }

            const bool isDefault = (defaultState == state.name);
            ImGui::Text("Default: %s", isDefault ? "Yes" : "No");

            if (ImGui::Button("Set As Default"))
            {
                defaultState = state.name;
                m_stateMachineDirty = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Preview State"))
            {
                previewState(state.name);
            }

            ImGui::SameLine();

            if (ImGui::Button("Delete State"))
            {
                states.erase(states.begin() + static_cast<std::ptrdiff_t>(i));
                ImGui::TreePop();
                ImGui::PopID();
                sanitizeStateMachineAfterEdit();
                m_stateMachineDirty = true;
                return;
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    if (ImGui::Button("Add State"))
    {
        AnimationStateMachineState state;
        state.name = "NewState";
        state.clipName.clear();
        state.speed = 1.0f;
        states.push_back(std::move(state));
        sanitizeStateMachineAfterEdit();
        m_stateMachineDirty = true;
    }
}
void AnimationComponent::drawTransitionsUi()
{
    if (!m_stateMachineAsset)
        return;

    auto& transitions = m_stateMachineAsset->getTransitionsMutable();

    if (!ImGui::CollapsingHeader("Transitions", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    for (size_t i = 0; i < transitions.size(); ++i)
    {
        AnimationStateMachineTransition& transition = transitions[i];
        ImGui::PushID(static_cast<int>(i));

        if (ImGui::TreeNode("Transition", "Transition %zu", i))
        {
            std::string oldSource = transition.sourceStateName;
            drawStateCombo("Source", transition.sourceStateName);
            if (transition.sourceStateName != oldSource)
            {
                m_stateMachineDirty = true;
            }

            std::string oldTarget = transition.targetStateName;
            drawStateCombo("Target", transition.targetStateName);
            if (transition.targetStateName != oldTarget)
            {
                m_stateMachineDirty = true;
            }

            if (InputTextString("Trigger", transition.triggerName))
            {
                m_stateMachineDirty = true;
            }

            if (ImGui::DragFloat("Blend Time", &transition.blendTimeSeconds, 0.01f, 0.0f, 10.0f))
            {
                m_stateMachineDirty = true;
            }

            if (ImGui::Button("Delete Transition"))
            {
                transitions.erase(transitions.begin() + static_cast<std::ptrdiff_t>(i));
                ImGui::TreePop();
                ImGui::PopID();
                sanitizeStateMachineAfterEdit();
                m_stateMachineDirty = true;
                return;
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    if (ImGui::Button("Add Transition"))
    {
        const auto& states = m_stateMachineAsset->getStates();

        if (states.empty())
            return;

        AnimationStateMachineTransition transition;
        transition.sourceStateName = states.front().name;
        transition.targetStateName = (states.size() > 1) ? states[1].name : states.front().name;
        transition.triggerName = "trigger";
        transition.blendTimeSeconds = 0.25f;

        transitions.push_back(std::move(transition));
        sanitizeStateMachineAfterEdit();
        m_stateMachineDirty = true;
    }
}
void AnimationComponent::debugDrawRecursive(GameObject* go)
{
    if (!go)
        return;

    Transform* transform = go->GetTransform();
    if (!transform)
        return;

    const Matrix& worldMatrix = transform->getGlobalMatrix();
    const Vector3 worldPos = worldMatrix.Translation();

    Transform* parentTransform = transform->getRoot();
    if (parentTransform)
    {
        const Vector3 parentWorldPos = parentTransform->getGlobalMatrix().Translation();

        // Parent -> child
        DebugLine(parentWorldPos, worldPos, Vector3(1.0f, 1.0f, 1.0f), false);
    }

    drawAxisTriad(worldMatrix, 0.15f);

    const auto& children = transform->getAllChildren();
    for (GameObject* child : children)
    {
        if (child)
            debugDrawRecursive(child);
    }
}

void AnimationComponent::drawAxisTriad(const Matrix& worldMatrix, float axisLength)
{
    const Vector3 origin = worldMatrix.Translation();

    const Vector3 right(worldMatrix._11, worldMatrix._12, worldMatrix._13);
    const Vector3 up(worldMatrix._21, worldMatrix._22, worldMatrix._23);
    const Vector3 forward(worldMatrix._31, worldMatrix._32, worldMatrix._33);

    Vector3 x = right;
    Vector3 y = up;
    Vector3 z = forward;

    if (x.LengthSquared() > 0.0f) x.Normalize();
    if (y.LengthSquared() > 0.0f) y.Normalize();
    if (z.LengthSquared() > 0.0f) z.Normalize();

    DebugLine(origin, origin + x * axisLength, Vector3(1.0f, 0.0f, 0.0f), true);
    DebugLine(origin, origin + y * axisLength, Vector3(0.0f, 1.0f, 0.0f), true);
    DebugLine(origin, origin + z * axisLength, Vector3(0.0f, 0.0f, 1.0f), true);
}

void AnimationComponent::drawUi()
{
    char uidBuffer[128];
    std::strncpy(uidBuffer, m_stateMachineUIDInput.c_str(), sizeof(uidBuffer));
    uidBuffer[sizeof(uidBuffer) - 1] = '\0';

    if (ImGui::InputText("State Machine UID", uidBuffer, sizeof(uidBuffer)))
    {
        m_stateMachineUIDInput = uidBuffer;
    }

    if (ImGui::Button("Apply State Machine UID"))
    {
        setStateMachineUID(m_stateMachineUIDInput);
    }

    ImGui::SameLine();

    if (ImGui::Button("Save State Machine"))
    {
        saveStateMachineAsset();
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(m_stateMachineUID == INVALID_ASSET_ID);
    if (ImGui::Button("Open State Machine Editor"))
    {
        ModuleEditor* moduleEditor = app ? app->getModuleEditor() : nullptr;
        WindowAnimationStateMachine* stateMachineWindow =
            moduleEditor ? moduleEditor->getWindowAnimationStateMachine() : nullptr;

        if (stateMachineWindow)
        {
            stateMachineWindow->setTargetStateMachineUID(m_stateMachineUID);
            stateMachineWindow->setOpen(true);
        }
    }
    ImGui::EndDisabled();

    ImGui::Text("State Machine Dirty: %s", m_stateMachineDirty ? "Yes" : "No");

    ImGui::Text("Active State: %s", m_activeStateName.empty() ? "<none>" : m_activeStateName.c_str());
    ImGui::Text("Duration: %.3f", m_controller.GetDuration());
    ImGui::Text("Current Time: %.3f", m_controller.GetTime());
    ImGui::Text("Speed: %.3f", m_controller.GetSpeed());


    ImGui::Checkbox("Play On Start", &m_playOnStart);
    ImGui::Checkbox("Apply Scale", &m_applyScale);
    ImGui::Checkbox("Force World Update", &m_forceWorldAfterApply);
    ImGui::Checkbox("Debug Draw Hierarchy", &m_debugDrawHierarchy);

    const char* stateText = m_controller.IsPlaying() ? "Playing" : "Stopped / Paused";
    ImGui::Text("Playback: %s", stateText);

    if (ImGui::Button("Play"))
    {
        if (ensureStateMachineLoaded())
        {
            if (m_activeStateName.empty())
            {
                startStateMachineIfNeeded();
            }
            else
            {
                m_controller.Play(m_controller.IsLooping());
                m_hasStartedPlayback = true;
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Pause"))
    {
        m_controller.Pause();
        m_hasStartedPlayback = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop"))
    {
        m_controller.Stop();
        m_hasStartedPlayback = true;
    }


    // TEST
    char triggerBuffer[128];
    std::strncpy(triggerBuffer, m_triggerInput.c_str(), sizeof(triggerBuffer));
    triggerBuffer[sizeof(triggerBuffer) - 1] = '\0';

    if (ImGui::InputText("Trigger", triggerBuffer, sizeof(triggerBuffer)))
    {
        m_triggerInput = triggerBuffer;
    }

    if (ImGui::Button("Send Trigger"))
    {
        SendTrigger(m_triggerInput);
    }

    ImGui::Text("Fade Time: %.3f", m_currentFadeTime);
    ImGui::Text("Transition Time: %.3f", m_currentTransitionTime);

    drawStateMachineResourceUi();

}

rapidjson::Value AnimationComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", static_cast<int>(getType()), domTree.GetAllocator());
    componentInfo.AddMember("Active", isActive(), domTree.GetAllocator());

    rapidjson::Value stateMachineUIDValue(m_stateMachineUID.c_str(), domTree.GetAllocator());
    componentInfo.AddMember("StateMachineUID", stateMachineUIDValue, domTree.GetAllocator());

    componentInfo.AddMember("PlayOnStart", m_playOnStart, domTree.GetAllocator());
    componentInfo.AddMember("ApplyScale", m_applyScale, domTree.GetAllocator());
    componentInfo.AddMember("ForceWorldAfterApply", m_forceWorldAfterApply, domTree.GetAllocator());

    return componentInfo;
}

bool AnimationComponent::deserializeJSON(const rapidjson::Value& componentValue)
{
    if (componentValue.HasMember("StateMachineUID") && componentValue["StateMachineUID"].IsString())
        m_stateMachineUID = componentValue["StateMachineUID"].GetString();
    else
        m_stateMachineUID = INVALID_ASSET_ID;

    if (componentValue.HasMember("PlayOnStart") && componentValue["PlayOnStart"].IsBool())
        m_playOnStart = componentValue["PlayOnStart"].GetBool();
    else
        m_playOnStart = true;

    if (componentValue.HasMember("ApplyScale") && componentValue["ApplyScale"].IsBool())
        m_applyScale = componentValue["ApplyScale"].GetBool();
    else
        m_applyScale = false;

    if (componentValue.HasMember("ForceWorldAfterApply") && componentValue["ForceWorldAfterApply"].IsBool())
        m_forceWorldAfterApply = componentValue["ForceWorldAfterApply"].GetBool();
    else
        m_forceWorldAfterApply = true;

    m_stateMachineAsset.reset();
    resetRuntime();
    m_stateMachineUIDInput = m_stateMachineUID;
    m_triggerInput.clear();

    m_stateMachineDirty = false;

    return true;
}

void AnimationComponent::setStateMachineUID(const MD5Hash& uid)
{
    if (m_stateMachineUID == uid)
        return;

    m_stateMachineUID = uid;
    m_stateMachineUIDInput = m_stateMachineUID;

    resetRuntime();
    m_stateMachineAsset.reset();

    m_stateMachineDirty = false;
}

bool AnimationComponent::SendTrigger(const std::string& triggerName)
{
    if (triggerName.empty())
        return false;

    if (!ensureStateMachineLoaded())
        return false;

    if (m_activeStateName.empty())
    {
        startStateMachineIfNeeded();
    }

    const AnimationStateMachineTransition* transition = findTransitionByTrigger(triggerName);
    if (!transition)
        return false;

    return activateState(transition->targetStateName, true, transition->blendTimeSeconds);
}

bool AnimationComponent::ensureStateMachineLoaded()
{
    if (m_stateMachineUID == INVALID_ASSET_ID)
        return false;

    if (m_stateMachineAsset)
        return true;

    ModuleAssets* moduleAssets = app ? app->getModuleAssets() : nullptr;
    if (!moduleAssets)
        return false;

    m_stateMachineAsset = moduleAssets->load<AnimationStateMachineAsset>(m_stateMachineUID);
    if (!m_stateMachineAsset)
    {
        DEBUG_WARN("[AnimationComponent] Could not load AnimationStateMachineAsset '%s'.", m_stateMachineUID.c_str());
        return false;
    }

    m_stateMachineDirty = false;

    return true;
}

void AnimationComponent::startStateMachineIfNeeded()
{
    if (!m_playOnStart)
        return;

    if (m_hasStartedPlayback)
        return;

    if (!ensureStateMachineLoaded())
        return;

    std::string defaultStateName = m_stateMachineAsset->getDefaultStateName();

    if (defaultStateName.empty())
    {
        const auto& states = m_stateMachineAsset->getStates();
        if (!states.empty())
        {
            defaultStateName = states.front().name;
        }
    }

    if (defaultStateName.empty())
        return;

    if (activateState(defaultStateName, true, 0.0f))
    {
        m_hasStartedPlayback = true;
    }
}


void AnimationComponent::resetRuntime()
{
    m_controller.Stop();
    m_controller.SetAnimation(std::shared_ptr<AnimationAsset>{});

    m_currentAnimationAsset.reset();
    m_activeStateName.clear();

    m_currentFadeTime = 0.0f;
    m_currentTransitionTime = 0.0f;
    m_previousPlayback.reset();

    m_hasStartedPlayback = false;
}

bool AnimationComponent::saveStateMachineAsset()
{
    if (!m_stateMachineAsset)
        return false;

    ModuleAssets* moduleAssets = app ? app->getModuleAssets() : nullptr;
    if (!moduleAssets)
        return false;

    if (!moduleAssets->saveAnimationStateMachine(m_stateMachineAsset))
        return false;

    m_stateMachineDirty = false;
    return true;
}

const AnimationStateMachineClip* AnimationComponent::findClipByName(const std::string& clipName) const
{
    if (!m_stateMachineAsset)
        return nullptr;

    for (const AnimationStateMachineClip& clip : m_stateMachineAsset->getClips())
    {
        if (clip.name == clipName)
            return &clip;
    }

    return nullptr;
}

const AnimationStateMachineState* AnimationComponent::findStateByName(const std::string& stateName) const
{
    if (!m_stateMachineAsset)
        return nullptr;

    for (const AnimationStateMachineState& state : m_stateMachineAsset->getStates())
    {
        if (state.name == stateName)
            return &state;
    }

    return nullptr;
}

const AnimationStateMachineTransition* AnimationComponent::findTransitionByTrigger(const std::string& triggerName) const
{
    if (!m_stateMachineAsset || m_activeStateName.empty())
        return nullptr;

    for (const AnimationStateMachineTransition& transition : m_stateMachineAsset->getTransitions())
    {
        if (transition.sourceStateName == m_activeStateName &&
            transition.triggerName == triggerName)
        {
            return &transition;
        }
    }

    return nullptr;
}

void AnimationComponent::drawStateCombo(const char* label, std::string& value)
{
    if (!m_stateMachineAsset)
        return;

    const auto& states = m_stateMachineAsset->getStates();
    const char* preview = value.empty() ? "<none>" : value.c_str();

    if (ImGui::BeginCombo(label, preview))
    {
        for (const AnimationStateMachineState& state : states)
        {
            const bool selected = (state.name == value);
            if (ImGui::Selectable(state.name.c_str(), selected))
            {
                value = state.name;
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}

void AnimationComponent::drawClipCombo(const char* label, std::string& value)
{
    if (!m_stateMachineAsset)
        return;

    const auto& clips = m_stateMachineAsset->getClips();
    const char* preview = value.empty() ? "<none>" : value.c_str();

    if (ImGui::BeginCombo(label, preview))
    {
        for (const AnimationStateMachineClip& clip : clips)
        {
            const bool selected = (clip.name == value);
            if (ImGui::Selectable(clip.name.c_str(), selected))
            {
                value = clip.name;
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}

void AnimationComponent::sanitizeStateMachineAfterEdit()
{
    if (!m_stateMachineAsset)
        return;

    auto& states = m_stateMachineAsset->getStatesMutable();
    auto& clips = m_stateMachineAsset->getClipsMutable();
    auto& transitions = m_stateMachineAsset->getTransitionsMutable();
    std::string& defaultState = m_stateMachineAsset->getDefaultStateNameMutable();

    auto stateExists = [&](const std::string& name) -> bool
        {
            return std::any_of(states.begin(), states.end(),
                [&](const AnimationStateMachineState& state) { return state.name == name; });
        };

    auto clipExists = [&](const std::string& name) -> bool
        {
            return std::any_of(clips.begin(), clips.end(),
                [&](const AnimationStateMachineClip& clip) { return clip.name == name; });
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
        std::remove_if(transitions.begin(), transitions.end(),
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

    if (!defaultState.empty() && !stateExists(defaultState))
    {
        defaultState.clear();
    }

    if (defaultState.empty() && !states.empty())
    {
        defaultState = states.front().name;
    }

    if (!m_activeStateName.empty() && !stateExists(m_activeStateName))
    {
        resetRuntime();
    }
}
