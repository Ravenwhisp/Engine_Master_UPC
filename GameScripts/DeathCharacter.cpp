#include "pch.h"
#include "DeathCharacter.h"
#include "DeathSound.h"
#include "EnemyDamageable.h"
#include "EnemyShadowMark.h"
#include "PlayerMovement.h"
#include "DeathConfig.h"
#include "DeathBasicAttack.h"
#include "DeathChargedAttack.h"
#include "DeathTaunt.h"

#include "PersistingCheckpointState.h"

#include <cmath>
#include <vector>

IMPLEMENT_SCRIPT_FIELDS(DeathCharacter,
    SERIALIZED_ASSET_REF(m_config, "Death Config", AssetType::DATA_CONTAINER)
)


DeathCharacter::DeathCharacter(GameObject* owner)
    : CharacterBase(owner)
{
}

void DeathCharacter::Start()
{
    {
        SCRIPT_PROFILE_SCOPE("Start: CharacterBase");
        CharacterBase::Start();
    }

    {
        SCRIPT_PROFILE_SCOPE("Start: find sibling scripts");
        m_basicAttack = GameObjectAPI::findScript<DeathBasicAttack>(getOwner());
        m_chargedAttack = GameObjectAPI::findScript<DeathChargedAttack>(getOwner());
        m_specialAbility = GameObjectAPI::findScript<DeathTaunt>(getOwner());
        m_sound = GameObjectAPI::findScript<DeathSound>(getOwner());
        m_movement = GameObjectAPI::findScript<PlayerMovement>(getOwner());
    }
    if (m_basicAttack == nullptr)
    {
        Debug::warn("[DeathCharacter] DeathBasicAttack not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_chargedAttack == nullptr)
    {
        Debug::warn("[DeathCharacter] DeathChargedAttack not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_specialAbility == nullptr)
    {
        Debug::warn("[DeathCharacter] DeathTaunt not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_sound == nullptr)
    {
        Debug::log("[DeathCharacter] DeathSound not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_movement == nullptr)
    {
        Debug::log("[DeathCharacter] PlayerMovement not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (!PersistingCheckpointState::Get().IsStartOfLevel())
    {
        SCRIPT_PROFILE_SCOPE("Start: restore checkpoint");
        TransformAPI::setGlobalPosition(
            GameObjectAPI::getTransform(m_owner), PersistingCheckpointState::Get().m_savedDeathRespawn);
    }
}

void DeathCharacter::Update()
{
    bool downed = false;
    {
        SCRIPT_PROFILE_SCOPE("Update: check downed");
        downed = isDowned();
    }

    if (downed)
    {
        if (m_sound != nullptr)
        {
            SCRIPT_PROFILE_SCOPE("Downed: stop sound loops");
            m_sound->stopAllLoops();
        }
        {
            SCRIPT_PROFILE_SCOPE("Downed: reset combo");
            resetCombo();
        }
        return;
    }

    {
        SCRIPT_PROFILE_SCOPE("Update: tick combo");
        tickCombo(Time::getDeltaTime());
    }

    if (m_sound != nullptr && m_movement != nullptr)
    {
        SCRIPT_PROFILE_SCOPE("Update: hover sound");
        m_sound->setHoverActive(m_movement->isMoving());
    }
}

float DeathCharacter::getComboWindowR2() const
{
    return m_config.get()->m_comboWindowR2;
}

float DeathCharacter::getComboWindowMaxCharge() const
{
    return m_config.get()->m_comboWindowMaxCharge;
}

void DeathCharacter::tickCombo(float dt)
{
    if (m_comboCooldownTimer > 0.0f)
    {
        m_comboCooldownTimer -= dt;
    }

    if (m_comboStep == 0)
    {
        return;
    }

    // The combo window is for the time BETWEEN hits, not during them.
    // While the character is locked in an attack (or about to fire a buffered
    // one), the combo must not expire — otherwise long lock durations relative
    // to the window would reset the combo before the next input fires.
    bool usingAbility = false;
    {
        SCRIPT_PROFILE_SCOPE("Combo: check ability state");
        usingAbility = isUsingAbility();
    }

    if (usingAbility)
    {
        return;
    }

    m_comboTimer += dt;
    if (m_comboTimer >= m_activeComboWindow)
    {
        resetCombo();
    }
}

void DeathCharacter::advanceCombo(bool isR2, float comboWindowOverride)
{
    m_comboTimer = 0.0f;
    m_activeComboWindow = (comboWindowOverride > 0.0f) ? comboWindowOverride : m_config.get()->m_comboWindow;

    if (isR2)
    {
        m_consecutiveR2Count++;
    }
    else
    {
        m_consecutiveR2Count = 0;
    }

    m_comboStep++;

    if (m_comboStep >= 3)
    {
        resetCombo();
        m_comboCooldownTimer = m_config.get()->m_comboCooldown;
    }
}

void DeathCharacter::resetCombo()
{
    m_comboStep = 0;
    m_consecutiveR2Count = 0;
    m_comboTimer = 0.0f;
}

IMPLEMENT_SCRIPT(DeathCharacter)
