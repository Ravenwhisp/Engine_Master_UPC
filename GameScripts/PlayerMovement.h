#pragma once

#include "ScriptAPI.h"

class PlayerAnimationController;

class PlayerMovement : public Script
{
    DECLARE_SCRIPT(PlayerMovement)

public:
    explicit PlayerMovement(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    inline void playerMovement(GameObject* owner, const Vector3& timeStep) const 
    { 
        moveInternal(owner, timeStep * m_moveSpeed); 
    }
	inline void playerDashMovement(GameObject* owner, const Vector3& displacement) const 
    { 
        moveInternal(owner, displacement);
    }

    void setMoving(bool isMoving);
    bool isMoving() const { return m_isMoving; }

public:
    float m_moveSpeed = 3.5f;

    bool m_constrainToNavMesh = true;
    Vector3 m_navExtents = Vector3(2.0f, 4.0f, 2.0f);

private:
    void moveInternal(GameObject* owner, const Vector3& desiredPos) const;
    void applyTranslation(Transform* transform, const Vector3& currentPos, const Vector3& desiredPos) const;
    PlayerAnimationController* findAnimationController();

private:
    bool m_isMoving = false;
    PlayerAnimationController* m_playerAnimationController = nullptr;

};