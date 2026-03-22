#pragma once

#include "ScriptAPI.h"
#include "ScriptAutoRegister.h"
#include "ScriptFieldInfo.h"
#include "ScriptComponentRef.h"

class Transform;

class CameraFollow : public Script
{
    DECLARE_SCRIPT(CameraFollow)

public:
    explicit CameraFollow(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

public:
    ScriptComponentRef<Transform> m_firstTarget;
    ScriptComponentRef<Transform> m_secondTarget;

    Vector3 m_transformOffset = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_rotationOffset = Vector3(0.0f, 0.0f, 0.0f);

    float m_followSharpness = 12.0f;
    float m_zoomSharpness = 8.0f;

    float m_zoomStartDistance = 0.0f;
    float m_zoomEndDistance = 0.0f;
    float m_maxExtraHeight = 0.0f;

private:
    Vector3 computeFollowPoint() const;
    float computeTargetExtraHeight(const Vector3& p1, const Vector3& p2) const;
    float smoothExtraHeight(float current, float target, float sharpness, float dt) const;
    Vector3 computeDesiredCameraPosition(const Vector3& followPoint) const;
    Vector3 smoothCameraPosition(const Vector3& current, const Vector3& target, float sharpness, float dt) const;

    Vector3 lerpVector(const Vector3& start, const Vector3& end, float alpha) const;
    float lerpFloat(float start, float end, float alpha) const;

private:
    bool m_firstUpdateAfterResolve = true;

    float m_currentExtraHeight = 0.0f;
};