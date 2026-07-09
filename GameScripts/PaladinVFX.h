#pragma once

#include "ScriptAPI.h"

class PaladinVFX : public Script
{
    DECLARE_SCRIPT(PaladinVFX)

public:

    explicit PaladinVFX(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    void setWalkingDustActive(bool active);
    void stopWalkingDust();

    void startChargeAttackEffect();
    void stopChargeAttackEffect();

    void playBasicAttackEffect();

private:

    Vector3 getWalkingDustPosition() const;
    Vector3 getOwnerRotation() const;
    Vector3 getChargeAttackEffectPosition() const;
    Vector3 getBasicAttackEffectPosition() const;

    void addWalkingDust();
    void removeWalkingDust();
    void updateWalkingDustPosition();

    void addChargeAttackEffect();
    void removeChargeAttackEffect();
    void updateChargeAttackEffectPosition();

    void addBasicAttackEffect();
    void removeBasicAttackEffect();
    void updateBasicAttackEffectLifetime(float deltaTime);

public:

    PrefabRef m_walkingDustPrefab;
    PrefabRef m_chargeAttackEffectPrefab;
    PrefabRef m_basicAttackEffectPrefab;

    float walkingDustYOffset = 0.05f;
    float walkingDustForwardOffset = -0.35f;

private:

    GameObject* walkingDustEffect = nullptr;
    bool walkingDustActive = false;

    GameObject* chargeAttackEffect = nullptr;
    bool chargeAttackEffectActive = false;

    float chargeAttackYOffset = 0.5f;
    float chargeAttackForwardOffset = 0.0f;

    GameObject* basicAttackEffect = nullptr;
    float basicAttackYOffset = 0.05f;
    float basicAttackForwardOffset = 0.75f;
    float basicAttackEffectLifetime = 1.0f;
    float basicAttackEffectTimer = 0.0f;

};