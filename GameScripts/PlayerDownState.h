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
    void enterDefeatedState();

    bool isDowned() const;
    float getReviveProgress() const;
    void blockRevive();

private:
    bool isTeammateInAssistRange() const;
    void completeRevive();

public:
    float m_selfReviveTime = 10.0f;
    float m_assistRadius = 3.0f;
    float m_assistSpeedMultiplier = 2.0f;
    float m_reviveHp = 50.0f;

    ScriptComponentRef<Transform> m_teammateTransform;

    ScriptComponentRef<Transform> m_downedSprite;
    GameObject* m_downedSpriteGO = nullptr;

private:
    PlayerState* m_playerState = nullptr;
    Damageable* m_damageable = nullptr;

    float m_reviveProgress = 0.0f;

    bool m_reviveBlocked = false;
};