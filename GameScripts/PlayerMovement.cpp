#include "pch.h"
#include "PlayerMovement.h"
#include "PlayerAnimationController.h"

static const ScriptFieldInfo playerMovementFields[]
{
    { "Move Speed", ScriptFieldType::Float, offsetof(PlayerMovement, m_moveSpeed), { 0.0f, 50.0f, 0.05f } },
    { "Constrain To NavMesh", ScriptFieldType::Bool, offsetof(PlayerMovement, m_constrainToNavMesh) },
    { "Nav Extents", ScriptFieldType::Vec3, offsetof(PlayerMovement, m_navExtents) }
};

IMPLEMENT_SCRIPT_FIELDS(PlayerMovement, playerMovementFields)

PlayerMovement::PlayerMovement(GameObject* owner)
    : Script(owner)
{
}

void PlayerMovement::Start()
{
    m_playerAnimationController = findAnimationController();
}

void PlayerMovement::Update()
{
}

void PlayerMovement::setMoving(bool isMoving)
{
    if (m_isMoving == isMoving)
    {
        return;
    }

    m_isMoving = isMoving;

    if (m_playerAnimationController)
    {
        m_playerAnimationController->setMoving(m_isMoving);
    }
}

void PlayerMovement::moveInternal(GameObject* owner, const Vector3& displacement) const
{
    Transform* transform = GameObjectAPI::getTransform(owner);
    if (!transform) return;

    const Vector3 currentPos = TransformAPI::getPosition(transform);
    const Vector3 desiredPos = currentPos + displacement;

    applyTranslation(transform, currentPos, desiredPos);
}

void PlayerMovement::applyTranslation(Transform* transform, const Vector3& currentPos, const Vector3& desiredPos) const
{
    if (!m_constrainToNavMesh)
    {
        TransformAPI::setPosition(transform, desiredPos);
        return;
    }

    Vector3 constrainedPos;
    if (NavigationAPI::moveAlongSurface(currentPos, desiredPos, constrainedPos, m_navExtents))
    {
        TransformAPI::setPosition(transform, constrainedPos);
    }
}

PlayerAnimationController* PlayerMovement::findAnimationController()
{
    Script* animationScript = m_owner ? GameObjectAPI::getScript(m_owner, "PlayerAnimationController") : nullptr;
    if (animationScript)
    {
        return static_cast<PlayerAnimationController*>(animationScript);
    }

    Debug::warn("PlayerMovement on '%s' could not find PlayerAnimationController on the same GameObject.", GameObjectAPI::getName(m_owner));
    return nullptr;
}

IMPLEMENT_SCRIPT(PlayerMovement)