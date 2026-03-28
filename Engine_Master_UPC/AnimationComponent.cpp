#include "Globals.h"
#include "AnimationComponent.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleTime.h"

#include "AnimationAsset.h"
#include "AnimationStateMachineAsset.h"
#include "GameObject.h"
#include "Transform.h"

#include <imgui.h>
#include <cstring>

namespace
{
    void DebugLine(const Vector3& from, const Vector3& to, const Vector3& color, bool depthEnabled = false)
    {
        const float p0[3] = { from.x, from.y, from.z };
        const float p1[3] = { to.x, to.y, to.z };
        const float c[3] = { color.x, color.y, color.z };

        dd::line(p0, p1, c, 0, depthEnabled);
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
    m_controller.Stop();
    m_controller.SetAnimation(std::shared_ptr<AnimationAsset>{});
    //m_animationAsset.reset();
    m_hasStartedPlayback = false;
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

    m_controller.Update(moduleTime->deltaTime());

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

bool AnimationComponent::activateState(const std::string& stateName, bool autoPlay)
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

void AnimationComponent::applyRecursive(GameObject* go)
{
    if (!go)
        return;

    Transform* transform = go->GetTransform();
    if (!transform)
        return;

    AnimationSample sample;
    if (m_controller.GetTransform(go->GetName(), sample))
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

    return activateState(transition->targetStateName, true);
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

    if (activateState(defaultStateName, true))
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
    m_hasStartedPlayback = false;
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
