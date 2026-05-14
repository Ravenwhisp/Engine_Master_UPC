#pragma once

#include "ScriptAPI.h"

class PowerupCollectible : public Script
{
    DECLARE_SCRIPT(PowerupCollectible)

public:
    explicit PowerupCollectible(GameObject* owner);

    void Start() override;
    void Update() override;
    void OnTriggerEnter(GameObject* player) override;

    ScriptFieldList getExposedFields() const override;

private:
    bool canBeCollectedBy(GameObject* player) const;
    bool applyPowerup();

    void idleAnimation();

private:
    enum PowerUpTarget
    {
        LYRIEL = 0,
        DEATH,
        BOTH
    };

    enum PowerUpEffect
    {
        LYRIEL_POWERUP_1 = 0,
        DEATH_POWERUP_1
    };

    int m_targetCharacter = BOTH;
    int m_powerupEffect = LYRIEL_POWERUP_1;

    bool m_collected = false;

    // Idle animation
    Vector3 m_startPosition = Vector3::Zero;

    float m_idleTimer = 0.0f;
    float m_idleSpeed = 0.5f;
    float m_horizontalAmplitude = 0.1f;
    float m_verticalAmplitude = 0.2f;
};