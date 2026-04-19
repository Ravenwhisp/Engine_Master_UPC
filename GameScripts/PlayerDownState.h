#pragma once

#include "ScriptAPI.h"

class Damageable;
class Transform;
class PlayerState;

class PlayerDownState : public Script
{
    DECLARE_SCRIPT(PlayerDownState)

public:
    explicit PlayerDownState(GameObject* owner);

    void Start() override;
    void Update() override;
    ScriptFieldList getExposedFields() const override;

    void drawGizmo() override;

    void enterDownState();

    bool isDowned() const;
    float getReviveProgress() const;

private:
    Damageable* findDamageable() const;

    bool isTeammateInAssistRange() const;
    void completeRevive();

public:
    float m_selfReviveTime = 10.0f;
    float m_assistRadius = 3.0f;
    float m_assistSpeedMultiplier = 2.0f;
    float m_reviveHp = 50.0f;

    ScriptComponentRef<Transform> m_teammateTransform;

private:
    PlayerState* m_playerState = nullptr;
    Damageable* m_damageable = nullptr;

    float m_reviveProgress = 0.0f;
};