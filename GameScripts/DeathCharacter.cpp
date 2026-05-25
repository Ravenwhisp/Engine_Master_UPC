#include "pch.h"
#include "DeathCharacter.h"
#include "DeathSound.h"
#include "EnemyDamageable.h"
#include "EnemyShadowMark.h"
#include "PlayerMovement.h"

#include <cmath>
#include <vector>

IMPLEMENT_SCRIPT_FIELDS(DeathCharacter,
    SERIALIZED_FLOAT(m_comboWindow, "Combo Window R1", 0.1f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_comboWindowR2, "Combo Window R2", 0.1f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_comboWindowMaxCharge, "Combo Window Max Charge", 0.1f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_comboCooldown, "Combo Cooldown", 0.0f, 5.0f, 0.1f)
)

DeathCharacter::DeathCharacter(GameObject* owner)
    : CharacterBase(owner)
{
}

void DeathCharacter::Start()
{
    CharacterBase::Start();

    m_sound    = GameObjectAPI::findScript<DeathSound>(getOwner());
    m_movement = GameObjectAPI::findScript<PlayerMovement>(getOwner());

    if (m_sound == nullptr)
    {
        Debug::log("[DeathCharacter] DeathSound not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }
    if (m_movement == nullptr)
    {
        Debug::log("[DeathCharacter] PlayerMovement not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }
}

void DeathCharacter::Update()
{
    CharacterBase::Update();

    if (isDowned())
    {
        if (m_sound != nullptr)
        {
            m_sound->stopAllLoops();
        }
        resetCombo();
        return;
    }

    tickCombo(Time::getDeltaTime());

    if (m_sound != nullptr && m_movement != nullptr)
    {
        m_sound->setHoverActive(m_movement->isMoving());
    }
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
    if (isUsingAbility())
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
    m_comboTimer        = 0.0f;
    m_activeComboWindow = (comboWindowOverride > 0.0f) ? comboWindowOverride : m_comboWindow;

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
        m_comboCooldownTimer = m_comboCooldown;
    }
}

void DeathCharacter::resetCombo()
{
    m_comboStep          = 0;
    m_consecutiveR2Count = 0;
    m_comboTimer         = 0.0f;
}

IMPLEMENT_SCRIPT(DeathCharacter)
