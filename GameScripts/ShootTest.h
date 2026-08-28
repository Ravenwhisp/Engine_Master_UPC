#pragma once

#include "ScriptAPI.h"

struct ShootTestBullet
{
    bool active = false;
    Vector3 position = Vector3::Zero;
    Vector3 direction = Vector3::Zero;
    float traveled = 0.0f;
};

class ShootTest : public Script
{
    DECLARE_SCRIPT(ShootTest)

public:
    explicit ShootTest(GameObject* owner);

    void Update() override;

    FieldList getExposedFields() const override;

public:
    int m_playerIndex = 0;
    float m_shotSpeed = 15.0f;
    float m_shotRange = 10.0f;
    float m_shotRadius = 0.15f;

private:
    static const int MAX_SHOTS = 16;
    ShootTestBullet m_shots[MAX_SHOTS];

    void fireShot(const Vector3& origin, const Vector3& direction);
};
