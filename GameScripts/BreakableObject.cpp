#include "pch.h"
#include "BreakableObject.h"

#include "EnvironmentSound.h"
#include "ObjectVfxIds.h"
#include "ParticleLifecycle.h"

namespace
{
    constexpr const char* k_barrelBreak = "Play_Environment_Barrel_Break";
}

BreakableObject::BreakableObject(GameObject* owner)
    : Script(owner)
{
}

void BreakableObject::Start()
{
    m_isBroken = false;

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    m_normalObjectTransform = TransformAPI::findChildByName(ownerTransform, "Normal");
    m_brokenObjectTransform = TransformAPI::findChildByName(ownerTransform, "Broken");

    if (m_normalObjectTransform == nullptr)
    {
        Debug::warn("[BreakableObject] '%s' could not find child object named 'Normal'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_brokenObjectTransform == nullptr)
    {
        Debug::warn("[BreakableObject] '%s' could not find child object named 'Broken'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_normalObjectTransform != nullptr)
    {
        GameObject* normalObject = ComponentAPI::getOwner(m_normalObjectTransform);
        GameObjectAPI::setActive(normalObject, true);
    }

    if (m_brokenObjectTransform != nullptr)
    {
        GameObject* brokenObject = ComponentAPI::getOwner(m_brokenObjectTransform);
        GameObjectAPI::setActive(brokenObject, false);
    }

    m_navBlocker = NavigationAPI::getRuntimeBlockerComponent(getOwner());
    if (m_navBlocker != nullptr)
    {
        NavigationAPI::setBlocked(m_navBlocker, true);
    }
}

void BreakableObject::Update()
{
    m_timedBreakEffects.update(Time::getDeltaTime());
}

void BreakableObject::OnGameStop()
{
    m_timedBreakEffects.clear();
}

Vector3 BreakableObject::getBreakEffectPosition() const
{
    if (m_brokenObjectTransform != nullptr)
    {
        return TransformAPI::getGlobalPosition(m_brokenObjectTransform);
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform != nullptr)
    {
        return TransformAPI::getGlobalPosition(ownerTransform);
    }

    return Vector3::Zero;
}

void BreakableObject::spawnBreakBaseEffect()
{
    ParticleLifecycle::spawnOneShotTimed(
        m_timedBreakEffects,
        ObjectVfxIds::barrelBreakBase(),
        getBreakEffectPosition()
    );
}

void BreakableObject::onBreak()
{
    breakObject();
    EnvironmentSound::play(getOwner(), k_barrelBreak);
}

void BreakableObject::breakObject()
{
    if (m_isBroken)
    {
        return;
    }

    m_isBroken = true;

    if (m_normalObjectTransform != nullptr)
    {
        GameObject* normalObject = ComponentAPI::getOwner(m_normalObjectTransform);
        GameObjectAPI::setActive(normalObject, false);
    }

    if (m_brokenObjectTransform != nullptr)
    {
        GameObject* brokenObject = ComponentAPI::getOwner(m_brokenObjectTransform);
        GameObjectAPI::setActive(brokenObject, true);
    }

    spawnBreakBaseEffect();

    if (m_navBlocker != nullptr)
    {
        NavigationAPI::setBlocked(m_navBlocker, false);
    }

    Debug::log("[BreakableObject] '%s' broke.", GameObjectAPI::getName(getOwner()));
}

IMPLEMENT_SCRIPT(BreakableObject)
