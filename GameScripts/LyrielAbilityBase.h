#pragma once

#include "AbilityBase.h"

class LyrielCharacter;

class LyrielAbilityBase : public AbilityBase
{
public:
    explicit LyrielAbilityBase(GameObject* owner);

    void Start() override;
    void Update() override;

protected:
    Transform* findArrowSpawnTransform() const;
    void faceDirection(const Vector3& direction);
    Vector3 getFallbackFacingDirection() const;

    void beginAttackWindow(float lockDuration);
    void finishAttackWindow();

    void beginAttackPresentation();

    virtual void onAttackWindowUpdate() {}
    virtual void onAttackWindowFinished() {}

protected:
    LyrielCharacter* m_lyriel = nullptr;
    float m_attackStateTimer = 0.0f;
};