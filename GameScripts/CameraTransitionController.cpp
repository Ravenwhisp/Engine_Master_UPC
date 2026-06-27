#include "pch.h"
#include "CameraTransitionController.h"

#include "PlayerController.h"
#include "CameraFollow.h"
#include "CameraTransitionEvent.h"
#include "Damageable.h"
#include "HUDFader.h"

CameraTransitionController::CameraTransitionController(GameObject* owner)
    : Script(owner)
{
}

void CameraTransitionController::Start()
{
    m_cameraFollow = GameObjectAPI::findScript<CameraFollow>(getOwner());

    if (m_cameraFollow == nullptr)
    {
        Debug::warn("CameraTransitionController on '%s' could not find CameraFollow on the same GameObject.", GameObjectAPI::getName(getOwner()));
    }

    m_camera = CameraAPI::getCameraComponent(getOwner());

    if (m_camera == nullptr)
    {
        Debug::warn("CameraTransitionController on '%s' could not find CameraComponent on the same GameObject.", GameObjectAPI::getName(getOwner()));
    }

    findPlayerControllers();
    findHUDFader();
}

void CameraTransitionController::Update()
{
    if (!m_isTransitioning)
    {
        return;
    }

    const float dt = Time::getDeltaTime();

    switch (m_state)
    {
    case TransitionState::MovingToTarget:
        updateMovingToTarget(dt);
        break;

    case TransitionState::Holding:
        updateHolding(dt);
        break;

    case TransitionState::Returning:
        updateReturning(dt);
        break;

    case TransitionState::None:
    default:
        break;
    }
}

void CameraTransitionController::startTransition(CameraTransitionEvent* event)
{
    if (m_isTransitioning)
    {
        return;
    }

    startMovingToTarget(event);
}

void CameraTransitionController::startMovingToTarget(CameraTransitionEvent* event)
{
    Transform* cameraTransform = GameObjectAPI::getTransform(getOwner());

    m_currentEvent = event;
    m_isTransitioning = true;
    m_state = TransitionState::MovingToTarget;
    m_timer = 0.0f;

    m_transitionStartPosition = TransformAPI::getGlobalPosition(cameraTransform);
    m_transitionStartRotation = TransformAPI::getGlobalEulerDegrees(cameraTransform);

    if (m_camera != nullptr)
    {
        m_originalFov = CameraAPI::getFov(m_camera);
        m_returnStartFov = m_originalFov;
    }

    buildPathFromCurrentEvent();

    if (m_pathPositions.size() < 2)
    {
        finishTransition();
        return;
    }

    if (m_cameraFollow != nullptr)
    {
        m_cameraFollow->setFollowEnabled(false);
    }

    setPlayersGameplayInputLocked(true);
    setPlayersInvulnerable(true);

    if (m_hudFader != nullptr)
    {
        m_hudFader->fadeTo(0.0f, m_hudFadeOutDuration);
    }
}

void CameraTransitionController::updateMovingToTarget(float dt)
{
    Transform* cameraTransform = GameObjectAPI::getTransform(getOwner());

    const float totalDuration = m_currentEvent->getPathDuration();

    m_timer += dt;

    const float normalizedTime = m_timer / totalDuration;

    if (m_camera != nullptr && m_currentEvent->usesFovTransition())
    {
        const float newFov = MathAPI::lerp(m_originalFov, m_currentEvent->getTargetFov(), normalizedTime);

        CameraAPI::setFov(m_camera, newFov);
    }

    const Vector3 newPosition = evaluateCatmullRomPath(normalizedTime);
    const Vector3 newRotation = evaluateRotationPath(normalizedTime);

    TransformAPI::setGlobalPosition(cameraTransform, newPosition);
    TransformAPI::setGlobalRotationEuler(cameraTransform, newRotation);

    if (m_timer >= totalDuration)
    {
        TransformAPI::setGlobalPosition(cameraTransform, m_pathPositions.back());
        TransformAPI::setGlobalRotationEuler(cameraTransform, m_pathRotations.back());

        if (m_camera != nullptr && m_currentEvent->usesFovTransition())
        {
            CameraAPI::setFov(m_camera, m_currentEvent->getTargetFov());
        }

        m_state = TransitionState::Holding;
        m_timer = 0.0f;
    }
}

void CameraTransitionController::updateHolding(float dt)
{
    const float duration = m_currentEvent->getHoldDuration();

    m_timer += dt;

    if (m_timer < duration)
    {
        return;
    }

    Transform* cameraTransform = GameObjectAPI::getTransform(getOwner());

    m_returnStartPosition = TransformAPI::getGlobalPosition(cameraTransform);
    m_returnStartRotation = TransformAPI::getGlobalEulerDegrees(cameraTransform);

    if (m_camera != nullptr)
    {
        m_returnStartFov = CameraAPI::getFov(m_camera);
    }

    m_state = TransitionState::Returning;
    m_timer = 0.0f;
}

void CameraTransitionController::updateReturning(float dt)
{
    Transform* cameraTransform = GameObjectAPI::getTransform(getOwner());

    const float duration = m_currentEvent->getReturnDuration();

    m_timer += dt;

    const float normalizedTime = m_timer / duration;
    const float alpha = MathAPI::smoothStep(0.0f, 1.0f, normalizedTime);

    if (m_camera != nullptr && m_currentEvent->usesFovTransition())
    {
        const float newFov = MathAPI::lerp(m_returnStartFov, m_originalFov, alpha);

        CameraAPI::setFov(m_camera, newFov);
    }

    const Vector3 newPosition = MathAPI::lerp(m_returnStartPosition, m_transitionStartPosition, alpha);
    const Vector3 newRotation = MathAPI::lerp(m_returnStartRotation, m_transitionStartRotation, alpha);

    TransformAPI::setGlobalPosition(cameraTransform, newPosition);
    TransformAPI::setGlobalRotationEuler(cameraTransform, newRotation);

    if (m_timer >= duration)
    {
        TransformAPI::setGlobalPosition(cameraTransform, m_transitionStartPosition);
        TransformAPI::setGlobalRotationEuler(cameraTransform, m_transitionStartRotation);

        if (m_camera != nullptr && m_currentEvent->usesFovTransition())
        {
            CameraAPI::setFov(m_camera, m_originalFov);
        }

        finishTransition();
    }
}

void CameraTransitionController::finishTransition()
{
    if (m_cameraFollow != nullptr)
    {
        m_cameraFollow->setFollowEnabled(true);
    }

    setPlayersGameplayInputLocked(false);
    setPlayersInvulnerable(false);

    if (m_hudFader != nullptr)
    {
        m_hudFader->fadeTo(1.0f, m_hudFadeInDuration);
    }

    m_currentEvent = nullptr;
    m_state = TransitionState::None;
    m_isTransitioning = false;
    m_timer = 0.0f;
}

void CameraTransitionController::buildPathFromCurrentEvent()
{
    m_pathPositions.clear();
    m_pathRotations.clear();

    m_pathPositions.push_back(m_transitionStartPosition);
    m_pathRotations.push_back(m_transitionStartRotation);

    const int pointCount = m_currentEvent->getTargetPointCount();

    for (int i = 0; i < pointCount; ++i)
    {
        Transform* point = m_currentEvent->getTargetPoint(i);

        m_pathPositions.push_back(TransformAPI::getGlobalPosition(point));
        m_pathRotations.push_back(TransformAPI::getGlobalEulerDegrees(point));
    }
}

Vector3 CameraTransitionController::evaluateCatmullRomPath(float normalizedTime) const
{
    const int pointCount = static_cast<int>(m_pathPositions.size());
    const int segmentCount = pointCount - 1;

    const float scaledTime = normalizedTime * static_cast<float>(segmentCount);

    int segmentIndex = static_cast<int>(scaledTime);
    float localAlpha = scaledTime - static_cast<float>(segmentIndex);

    if (segmentIndex >= segmentCount)
    {
        segmentIndex = segmentCount - 1;
        localAlpha = 1.0f;
    }

    const int p0Index = segmentIndex > 0 ? segmentIndex - 1 : segmentIndex;
    const int p1Index = segmentIndex;
    const int p2Index = segmentIndex + 1;
    const int p3Index = segmentIndex + 2 < pointCount ? segmentIndex + 2 : segmentIndex + 1;

    return catmullRom(m_pathPositions[p0Index], m_pathPositions[p1Index], m_pathPositions[p2Index], m_pathPositions[p3Index], localAlpha);
}

Vector3 CameraTransitionController::evaluateRotationPath(float normalizedTime) const
{
    const int pointCount = static_cast<int>(m_pathRotations.size());
    const int segmentCount = pointCount - 1;

    const float scaledTime = normalizedTime * static_cast<float>(segmentCount);

    int segmentIndex = static_cast<int>(scaledTime);
    float localAlpha = scaledTime - static_cast<float>(segmentIndex);

    if (segmentIndex >= segmentCount)
    {
        segmentIndex = segmentCount - 1;
        localAlpha = 1.0f;
    }

    return MathAPI::lerp(m_pathRotations[segmentIndex], m_pathRotations[segmentIndex + 1], localAlpha);
}

Vector3 CameraTransitionController::catmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) const
{
    const float t2 = t * t;
    const float t3 = t2 * t;

    return (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 + (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
}

void CameraTransitionController::findPlayerControllers()
{
    m_playerControllers.clear();
    m_playerDamageables.clear();

    const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER, true);

    for (GameObject* player : players)
    {
        PlayerController* playerController = GameObjectAPI::findScript<PlayerController>(player);
        if (playerController == nullptr)
        {
            Debug::warn("CameraTransitionController could not find PlayerController on player '%s'.", GameObjectAPI::getName(player));
        }
        else
        {
            m_playerControllers.push_back(playerController);
        }

        Damageable* damageable = GameObjectAPI::findScript<Damageable>(player);
        if (damageable == nullptr)
        {
            Debug::warn("CameraTransitionController could not find Damageable on player '%s'.", GameObjectAPI::getName(player));
        }
        else
        {
            m_playerDamageables.push_back(damageable);
        }
    }
}

void CameraTransitionController::setPlayersGameplayInputLocked(bool locked)
{
    for (PlayerController* playerController : m_playerControllers)
    {
        if (playerController == nullptr)
        {
            continue;
        }

        playerController->setGameplayInputLocked(locked);
    }
}

void CameraTransitionController::setPlayersInvulnerable(bool invulnerable)
{
    for (Damageable* damageable : m_playerDamageables)
    {
        if (damageable == nullptr)
        {
            continue;
        }

        damageable->setInvulnerable(invulnerable);
    }
}

void CameraTransitionController::findHUDFader()
{
    const std::vector<GameObject*> hudFaderObjects = SceneAPI::findAllGameObjectsWithScript<HUDFader>();

    if (hudFaderObjects.empty())
    {
        Debug::warn("CameraTransitionController could not find any GameObject with HUDFader.");
        return;
    }

    m_hudFader = GameObjectAPI::findScript<HUDFader>(hudFaderObjects[0]);
}

IMPLEMENT_SCRIPT(CameraTransitionController)