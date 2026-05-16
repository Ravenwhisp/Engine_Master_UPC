#include "pch.h"
#include "PowerupCollectible.h"

#include "PersistingPowerupState.h"

#include <cmath>

static const char* powerupTargetNames[] =
{
    "Lyriel",
    "Death",
    "Both"
};

static const char* powerupEffectNames[] =
{
    "Lyriel Powerup 1",
    "Death Powerup 1"
};

IMPLEMENT_SCRIPT_FIELDS(PowerupCollectible,
    SERIALIZED_ENUM_INT(m_targetCharacter, "Target Character", powerupTargetNames, 3),
    SERIALIZED_ENUM_INT(m_powerupEffect, "Powerup Effect", powerupEffectNames, 2),
    SERIALIZED_FLOAT(m_idleSpeed, "Idle Speed", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_horizontalAmplitude, "Horizontal Amplitude", 0.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_verticalAmplitude, "Vertical Amplitude", 0.0f, 5.0f, 0.05f)
)

PowerupCollectible::PowerupCollectible(GameObject* owner)
    : Script(owner)
{
}

void PowerupCollectible::Start()
{
    m_startPosition = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner()));
}

void PowerupCollectible::Update()
{
    if (m_collected)
    {
        return;
    }

    idleAnimation();
}

void PowerupCollectible::OnTriggerEnter(GameObject* player)
{
    if (m_collected)
    {
        return;
    }

    if (!player || GameObjectAPI::getTag(player) != Tag::PLAYER)
    {
        return;
    }

    if (!canBeCollectedBy(player))
    {
        return;
    }

    if (!applyPowerup())
    {
        return;
    }

    m_collected = true;

    GameObjectAPI::removeGameObject(getOwner());
}

bool PowerupCollectible::canBeCollectedBy(GameObject* player) const
{
    const std::string& name = GameObjectAPI::getName(player);

    switch (m_targetCharacter)
    {
    case LYRIEL:
        return name == "Lyriel";

    case DEATH:
        return name == "Death";

    case BOTH:
        return name == "Lyriel" || name == "Death";

    default:
        return false;
    }
}

bool PowerupCollectible::applyPowerup()
{
    switch (m_powerupEffect)
    {
    case LYRIEL_POWERUP_1:
        PersistingPowerupState::unlock(PowerupId::LyrielPowerup1);
        Debug::log("[PowerupCollectible] Lyriel Powerup 1 unlocked.");
        return true;

    case DEATH_POWERUP_1:
        PersistingPowerupState::unlock(PowerupId::DeathPowerup1);
        Debug::log("[PowerupCollectible] Death Powerup 1 unlocked.");
        return true;

    default:
        return false;
    }
}

void PowerupCollectible::idleAnimation()
{
    m_idleTimer += Time::getDeltaTime();

    const float t = m_idleTimer * m_idleSpeed;

    Vector3 position = m_startPosition;

    position.z += std::sin(t) * m_horizontalAmplitude;
    position.y += std::sin(t * 2.0f) * m_verticalAmplitude;

    TransformAPI::setPosition(GameObjectAPI::getTransform(getOwner()), position);
}

IMPLEMENT_SCRIPT(PowerupCollectible)
