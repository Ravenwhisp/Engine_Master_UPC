#include "pch.h"
#include "SpikeTrap.h"

#include "EnvironmentSound.h"
#include "ParticleLifecycle.h"
#include "PlayerDamageable.h"

#include <algorithm>
#include <cstring>

IMPLEMENT_SCRIPT_FIELDS(SpikeTrap,
    SERIALIZED_BOOL(alternativeMode, "Alternative Mode"),
    SERIALIZED_FLOAT(a_duration, "Active Duration", 0.0f, 50.0f, 0.1f),
    SERIALIZED_FLOAT(p_duration, "Preparing Duration", 0.0f, 50.0f, 0.1f),
    SERIALIZED_FLOAT(extensionDuration, "Extension Duration", 0.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(retractionDuration, "Retraction Duration", 0.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(startPositionY, "Start Position Y", -10.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(waitPositionY, "Wait Position Y", -10.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(activePositionY, "Active Position Y", -10.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(trapDamage, "Trap Damage", 0.0f, 1000.0f, 1.0f),
    SERIALIZED_COMPONENT_REF(m_spikeShineT, "Spike Shine Particle", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_spectralAuraT, "Spectral Aura Particle", ComponentType::TRANSFORM)
)

SpikeTrap::SpikeTrap(GameObject* owner)
    : Script(owner)
{
}

void SpikeTrap::Start()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    m_normalSpike = TransformAPI::findChildByName(ownerTransform, "Normal");
    m_spectralSpike = TransformAPI::findChildByName(ownerTransform, "Spectral");

    const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);
    for (GameObject* player : players)
    {
        const char* name = GameObjectAPI::getName(player);
        if (name == nullptr)
        {
            continue;
        }

        if (std::strcmp(name, "Lyriel") == 0)
        {
            m_lyriel = player;
        }
        else if (std::strcmp(name, "Death") == 0)
        {
            m_death = player;
        }
    }

    if (m_normalSpike == nullptr || m_spectralSpike == nullptr)
    {
        Debug::warn("SpikeTrap '%s' requires children named Normal and Spectral.",
            GameObjectAPI::getName(getOwner()));
    }

    spikeType = alternativeMode ? 1 : 0;
    setSpikeHeight(0, spikeType == 0 ? waitPositionY : startPositionY);
    setSpikeHeight(1, spikeType == 1 ? waitPositionY : startPositionY);

    currentTime = 0.0f;
    state = WAIT;
    playersInside.clear();
    damagedPlayers.clear();
}

void SpikeTrap::Update()
{
    currentTime += Time::getDeltaTime();

    switch (state)
    {
    case WAIT:
        if (currentTime >= p_duration)
        {
            beginExtension();
        }
        break;

    case EXTENDING:
        updateExtension();
        break;

    case ACTIVE:
        if (currentTime >= a_duration)
        {
            beginRetraction();
        }
        break;

    case RETRACTING:
        updateRetraction();
        break;
    }
}

void SpikeTrap::OnTriggerEnter(GameObject* gameObject)
{
    if (gameObject == nullptr || GameObjectAPI::getTag(gameObject) != Tag::PLAYER)
    {
        return;
    }

    playersInside.insert(gameObject);

    // A player entering after the spikes are already extended is damaged now.
    if (state == ACTIVE && isCurrentTarget(gameObject))
    {
        damagePlayer(gameObject);
    }
}

void SpikeTrap::OnTriggerExit(GameObject* gameObject)
{
    if (gameObject == nullptr)
    {
        return;
    }

    playersInside.erase(gameObject);

    // Preserve the old behavior: leaving and re-entering during the same active
    // phase permits another hit.
    damagedPlayers.erase(gameObject);
}

void SpikeTrap::beginExtension()
{
    state = EXTENDING;
    currentTime = 0.0f;

    addEffect(spikeType);
    EnvironmentSound::playGrouped(
        getOwner(), "Play_Environment_Extend_Spikes", "SpikeTraps", 100);
}

void SpikeTrap::updateExtension()
{
    const float t = getAnimationProgress(extensionDuration);
    const float easedT = smoothStep(t);
    const float height = waitPositionY + (activePositionY - waitPositionY) * easedT;
    setSpikeHeight(spikeType, height);

    if (t >= 1.0f)
    {
        setSpikeHeight(spikeType, activePositionY);
        state = ACTIVE;
        currentTime = 0.0f;
        damagedPlayers.clear();

        // This is essential with trigger callbacks: a player may already be
        // inside when this spike type becomes dangerous.
        damagePlayersInside();
    }
}

void SpikeTrap::beginRetraction()
{
    state = RETRACTING;
    currentTime = 0.0f;

    removeEffect(spikeType);
    EnvironmentSound::playGrouped(
        getOwner(), "Play_Environment_Retract_Spikes", "SpikeTraps", 100);
}

void SpikeTrap::updateRetraction()
{
    const float t = getAnimationProgress(retractionDuration);
    const float easedT = smoothStep(t);
    const int nextSpikeType = spikeType == 0 ? 1 : 0;

    // Smoothly hide the outgoing side while bringing the incoming side up to
    // its telegraph height. Neither side deals damage during this transition.
    const float outgoingHeight = activePositionY + (startPositionY - activePositionY) * easedT;
    const float incomingHeight = startPositionY + (waitPositionY - startPositionY) * easedT;
    setSpikeHeight(spikeType, outgoingHeight);
    setSpikeHeight(nextSpikeType, incomingHeight);

    if (t >= 1.0f)
    {
        setSpikeHeight(spikeType, startPositionY);
        setSpikeHeight(nextSpikeType, waitPositionY);

        spikeType = nextSpikeType;
        state = WAIT;
        currentTime = 0.0f;
        damagedPlayers.clear();
    }
}

void SpikeTrap::setSpikeHeight(int type, float height)
{
    Transform* spike = type == 0 ? m_normalSpike : m_spectralSpike;
    if (spike == nullptr)
    {
        return;
    }

    Vector3 position = TransformAPI::getPosition(spike);
    position.y = height;
    TransformAPI::setPosition(spike, position);
}

float SpikeTrap::getAnimationProgress(float duration) const
{
    if (duration <= 0.0f)
    {
        return 1.0f;
    }

    return std::clamp(currentTime / duration, 0.0f, 1.0f);
}

float SpikeTrap::smoothStep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

bool SpikeTrap::isCurrentTarget(GameObject* player) const
{
    return spikeType == 0 ? player == m_lyriel : player == m_death;
}

void SpikeTrap::damagePlayersInside()
{
    for (GameObject* player : playersInside)
    {
        if (isCurrentTarget(player))
        {
            damagePlayer(player);
        }
    }
}

void SpikeTrap::damagePlayer(GameObject* player)
{
    if (player == nullptr || damagedPlayers.count(player) != 0)
    {
        return;
    }

    PlayerDamageable* damageable = GameObjectAPI::findScript<PlayerDamageable>(player);
    if (damageable == nullptr)
    {
        return;
    }

    damageable->takeDamage(trapDamage);
    damagedPlayers.insert(player);
}

void SpikeTrap::addEffect(int type)
{
    Transform* effectTransform = type == 0
        ? m_spikeShineT.getReferencedComponent()
        : m_spectralAuraT.getReferencedComponent();

    if (effectTransform != nullptr)
    {
        ParticleLifecycle::activate(ComponentAPI::getOwner(effectTransform));
    }
}

void SpikeTrap::removeEffect(int type)
{
    Transform* effectTransform = type == 0
        ? m_spikeShineT.getReferencedComponent()
        : m_spectralAuraT.getReferencedComponent();

    if (effectTransform != nullptr)
    {
        ParticleLifecycle::deactivate(ComponentAPI::getOwner(effectTransform));
    }
}

IMPLEMENT_SCRIPT(SpikeTrap)
