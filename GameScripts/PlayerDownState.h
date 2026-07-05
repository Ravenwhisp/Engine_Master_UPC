#pragma once

#include "ScriptAPI.h"

class Damageable;
class Transform;
class PlayerState;
class CooperativeSound;

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

    // Drives the cooperative revive loops. Reviving (downed, alone) and Help Reviving
    // (partner assisting) are mutually exclusive, matching the revive-UI swap.
    enum class ReviveAudioState { None, Self, Assisted };
    void setReviveAudio(ReviveAudioState state);

public:
    float m_selfReviveTime = 10.0f;
    float m_assistRadius = 3.0f;
    float m_assistSpeedMultiplier = 2.0f;
    float m_reviveHp = 50.0f;

    ScriptComponentRef<Transform> m_teammateTransform;

private:
    PlayerState* m_playerState = nullptr;
    Damageable* m_damageable = nullptr;
    CooperativeSound* m_cooperativeSound = nullptr;

    float m_reviveProgress = 0.0f;

    bool m_reviveBlocked = false;

    ReviveAudioState m_reviveAudioState = ReviveAudioState::None;

public:
    ScriptComponentRef<Transform> m_downedSprite;

private:
    Transform* m_downedSpriteTransform = nullptr;
	
	Transform* m_revivePivotTransform = nullptr;
    Transform* m_reviveSliderCanvas = nullptr;
    Transform* m_reviveHandleCanvas = nullptr;
    Transform* m_reviveIconCanvas = nullptr;

    UISlider* m_reviveSlider = nullptr;
    UISlider* m_reviveSlider2 = nullptr;

    Transform* m_reviveIconTransform = nullptr;
    Transform* m_reviveHandleTransform = nullptr;
	Transform* m_reviveIcon2Transform = nullptr;
    Transform* m_reviveHandle2Transform = nullptr;

    void setupUI();
	void updateReviveUI();

};