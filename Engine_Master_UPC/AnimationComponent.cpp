#include "Globals.h"
#include "AnimationComponent.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleTime.h"

#include "AnimationAsset.h"
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
    , m_animationUIDInput(m_animationUID)
{
}

std::unique_ptr<Component> AnimationComponent::clone(GameObject* newOwner) const
{
    auto cloned = std::make_unique<AnimationComponent>(m_uuid, newOwner);

    cloned->m_animationUID = m_animationUID;
    cloned->m_loop = m_loop;
    cloned->m_playOnStart = m_playOnStart;
    cloned->m_applyScale = m_applyScale;
    cloned->m_forceWorldAfterApply = m_forceWorldAfterApply;

    cloned->setActive(isActive());
    return cloned;
}

bool AnimationComponent::init()
{
    ensureAnimationLoaded();
    startPlaybackIfNeeded();
    return true;

}

bool AnimationComponent::cleanUp()
{
    m_controller.Stop();
    m_controller.SetAnimation(std::shared_ptr<AnimationAsset>{});
    m_animationAsset.reset();
    m_hasStartedPlayback = false;
    return true;
}

void AnimationComponent::setAnimationUID(const MD5Hash& uid)
{
    if (m_animationUID == uid)
        return;

    m_animationUID = uid;
    m_animationUIDInput = m_animationUID;

    m_controller.Stop();
    m_controller.SetAnimation(std::shared_ptr<AnimationAsset>{});

    m_animationAsset.reset();
    m_hasStartedPlayback = false;
}

bool AnimationComponent::ensureAnimationLoaded()
{
    if (m_animationUID == INVALID_ASSET_ID)
        return false;

    if (m_animationAsset)
        return true;

    ModuleAssets* moduleAssets = app->getModuleAssets();
    if (!moduleAssets)
        return false;

    m_animationAsset = moduleAssets->load<AnimationAsset>(m_animationUID);
    if (!m_animationAsset)
    {
        DEBUG_WARN("[AnimationComponent] Could not load AnimationAsset '%s'.", m_animationUID.c_str());
        return false;
    }

    m_controller.SetAnimation(m_animationAsset);
    return true;
}

void AnimationComponent::startPlaybackIfNeeded()
{
    if (!m_playOnStart)
        return;

    if (m_hasStartedPlayback)
        return;

    if (!m_animationAsset)
        return;

    m_controller.Play(m_loop);
    m_hasStartedPlayback = true;
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

    if (!ensureAnimationLoaded())
        return;

    startPlaybackIfNeeded();

    m_controller.SetLoop(m_loop);
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
    std::strncpy(uidBuffer, m_animationUIDInput.c_str(), sizeof(uidBuffer));
    uidBuffer[sizeof(uidBuffer) - 1] = '\0';

    if (ImGui::InputText("Animation UID", uidBuffer, sizeof(uidBuffer)))
    {
        m_animationUIDInput = uidBuffer;
    }

    if (ImGui::Button("Apply Animation UID"))
    {
        setAnimationUID(m_animationUIDInput);
    }

    ImGui::Text("Duration: %.3f", m_controller.GetDuration());
    ImGui::Text("Current Time: %.3f", m_controller.GetTime());

    ImGui::Checkbox("Loop", &m_loop);
    ImGui::Checkbox("Play On Start", &m_playOnStart);
    ImGui::Checkbox("Apply Scale", &m_applyScale);
    ImGui::Checkbox("Force World Update", &m_forceWorldAfterApply);
    ImGui::Checkbox("Debug Draw Hierarchy", &m_debugDrawHierarchy);

    const char* stateText = m_controller.IsPlaying() ? "Playing" : "Stopped / Paused";
    ImGui::Text("State: %s", stateText);

    if (ImGui::Button("Play"))
    {
        if (ensureAnimationLoaded())
        {
            m_controller.Play(m_loop);
            m_hasStartedPlayback = true;
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

}

rapidjson::Value AnimationComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", static_cast<int>(getType()), domTree.GetAllocator());
    componentInfo.AddMember("Active", isActive(), domTree.GetAllocator());

    rapidjson::Value animationUIDValue(m_animationUID.c_str(), domTree.GetAllocator());
    componentInfo.AddMember("AnimationUID", animationUIDValue, domTree.GetAllocator());

    componentInfo.AddMember("Loop", m_loop, domTree.GetAllocator());
    componentInfo.AddMember("PlayOnStart", m_playOnStart, domTree.GetAllocator());
    componentInfo.AddMember("ApplyScale", m_applyScale, domTree.GetAllocator());
    componentInfo.AddMember("ForceWorldAfterApply", m_forceWorldAfterApply, domTree.GetAllocator());

    return componentInfo;
}

bool AnimationComponent::deserializeJSON(const rapidjson::Value& componentValue)
{
    if (componentValue.HasMember("AnimationUID") && componentValue["AnimationUID"].IsString())
        m_animationUID = componentValue["AnimationUID"].GetString();
    else
        m_animationUID = INVALID_ASSET_ID;

    if (componentValue.HasMember("Loop") && componentValue["Loop"].IsBool())
        m_loop = componentValue["Loop"].GetBool();
    else
        m_loop = true;

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

    m_animationAsset.reset();
    m_controller.Stop();
    m_controller.SetAnimation(std::shared_ptr<AnimationAsset>{});
    m_hasStartedPlayback = false;
    m_animationUIDInput = m_animationUID;

    return true;
}