#include "pch.h"
#include "CameraTransitionController.h"

#include "PlayerController.h"
#include "CameraFollow.h"
#include "CameraTransitionEvent.h"
#include "CameraTransitionStep.h"
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
    case TransitionState::MovingStep:
        updateMovingStep(dt);
        break;

    case TransitionState::HoldingStep:
        updateHoldingStep(dt);
        break;

    case TransitionState::WaitingForRelease:
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
        if (m_currentEvent == event && event->isHoldWhileTriggeredMode() && m_state == TransitionState::Returning)
        {
            startTransitionSequence(event, true);
        }

        return;
    }

    startTransitionSequence(event, false);
}

void CameraTransitionController::releaseTransition(CameraTransitionEvent* event)
{
    if (!m_isTransitioning)
    {
        return;
    }

    if (m_currentEvent != event)
    {
        return;
    }

    if (!m_currentEvent->isHoldWhileTriggeredMode())
    {
        return;
    }

    if (m_state == TransitionState::WaitingForRelease || m_state == TransitionState::MovingStep || m_state == TransitionState::HoldingStep)
    {
        startReturning();
    }
}

void CameraTransitionController::startTransitionSequence(CameraTransitionEvent* event, bool preserveOriginalFov)
{
    Transform* cameraTransform = GameObjectAPI::getTransform(getOwner());

    m_currentEvent = event;
    m_isTransitioning = true;
    m_timer = 0.0f;
    m_currentStepIndex = -1;

    m_transitionStartPosition = TransformAPI::getGlobalPosition(cameraTransform);
    m_transitionStartRotation = TransformAPI::getGlobalEulerDegrees(cameraTransform);

    if (m_camera != nullptr && !preserveOriginalFov)
    {
        m_originalFov = CameraAPI::getFov(m_camera);
        m_returnStartFov = m_originalFov;
    }

    if (!hasValidStepSequence())
    {
        Debug::warn("CameraTransitionController could not start transition '%s' because it has no CameraTransitionStep points.", GameObjectAPI::getName(event->getOwner()));

        finishTransition();
        return;
    }

    if (m_cameraFollow != nullptr)
    {
        m_cameraFollow->setFollowEnabled(false);
    }

    if (m_currentEvent->shouldLockGameplayInput())
    {
        setPlayersGameplayInputLocked(true);
    }

    if (m_currentEvent->shouldMakePlayersInvulnerable())
    {
        setPlayersInvulnerable(true);
    }

    if (m_currentEvent->shouldFadeHud() && m_hudFader != nullptr)
    {
        m_hudFader->fadeTo(0.0f, m_hudFadeOutDuration);
    }

    startStep(0);
}

void CameraTransitionController::startStep(int stepIndex)
{
    if (m_currentEvent == nullptr)
    {
        finishTransition();
        return;
    }

    CameraTransitionStep* step = m_currentEvent->getTransitionStep(stepIndex);
    Transform* point = m_currentEvent->getTargetPoint(stepIndex);

    if (step == nullptr || point == nullptr)
    {
        Debug::warn("CameraTransitionController could not start step %d for transition '%s'.", stepIndex, GameObjectAPI::getName(m_currentEvent->getOwner()));

        startReturning();
        return;
    }

    Transform* cameraTransform = GameObjectAPI::getTransform(getOwner());

    m_currentStepIndex = stepIndex;
    m_timer = 0.0f;

    m_stepStartPosition = TransformAPI::getGlobalPosition(cameraTransform);
    m_stepStartRotation = TransformAPI::getGlobalEulerDegrees(cameraTransform);

    m_stepTargetPosition = TransformAPI::getGlobalPosition(point);
    m_stepTargetRotation = TransformAPI::getGlobalEulerDegrees(point);

    m_stepMoveDuration = step->getMoveDuration();
    m_stepHoldDuration = step->getHoldDuration();

    m_stepUsesFovTransition = step->usesFovTransition();

    if (m_camera != nullptr)
    {
        m_stepStartFov = CameraAPI::getFov(m_camera);
        m_stepTargetFov = m_stepUsesFovTransition ? step->getTargetFov() : m_stepStartFov;
    }

    m_state = TransitionState::MovingStep;

    step->executeStepStartedActions(this);
}

void CameraTransitionController::startReturning()
{
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

void CameraTransitionController::updateMovingStep(float dt)
{
    Transform* cameraTransform = GameObjectAPI::getTransform(getOwner());

    if (m_stepMoveDuration <= 0.0001f)
    {
        finishCurrentStepMovement();
        return;
    }

    m_timer += dt;

    float normalizedTime = m_timer / m_stepMoveDuration;
    if (normalizedTime > 1.0f)
    {
        normalizedTime = 1.0f;
    }

    const Vector3 newPosition = evaluateStepPosition(normalizedTime);
    const Vector3 newRotation = evaluateStepRotation(normalizedTime);

    TransformAPI::setGlobalPosition(cameraTransform, newPosition);
    TransformAPI::setGlobalRotationEuler(cameraTransform, newRotation);

    if (m_camera != nullptr && m_stepUsesFovTransition)
    {
        const float newFov = MathAPI::lerp(m_stepStartFov, m_stepTargetFov, normalizedTime);
        CameraAPI::setFov(m_camera, newFov);
    }

    if (m_timer >= m_stepMoveDuration)
    {
        finishCurrentStepMovement();
    }
}

void CameraTransitionController::updateHoldingStep(float dt)
{
    if (m_stepHoldDuration <= 0.0001f)
    {
        finishCurrentStepHold();
        return;
    }

    m_timer += dt;

    if (m_timer < m_stepHoldDuration)
    {
        return;
    }

    finishCurrentStepHold();
}

void CameraTransitionController::updateReturning(float dt)
{
    Transform* cameraTransform = GameObjectAPI::getTransform(getOwner());

    const float duration = m_currentEvent->getReturnDuration();

    m_timer += dt;

    const float normalizedTime = m_timer / duration;
    const float alpha = MathAPI::smoothStep(0.0f, 1.0f, normalizedTime);

    if (m_camera != nullptr)
    {
        const float newFov = MathAPI::lerp(m_returnStartFov, m_originalFov, alpha);
        CameraAPI::setFov(m_camera, newFov);
    }

    Vector3 targetPosition = m_transitionStartPosition;
    Vector3 targetRotation = m_transitionStartRotation;

    getCameraFollowReturnTarget(targetPosition, targetRotation);

    const Vector3 newPosition = MathAPI::lerp(m_returnStartPosition, targetPosition, alpha);
    const Vector3 newRotation = MathAPI::lerp(m_returnStartRotation, targetRotation, alpha);

    TransformAPI::setGlobalPosition(cameraTransform, newPosition);
    TransformAPI::setGlobalRotationEuler(cameraTransform, newRotation);

    if (m_timer >= duration)
    {
        Vector3 targetPosition = m_transitionStartPosition;
        Vector3 targetRotation = m_transitionStartRotation;

        getCameraFollowReturnTarget(targetPosition, targetRotation);

        TransformAPI::setGlobalPosition(cameraTransform, targetPosition);
        TransformAPI::setGlobalRotationEuler(cameraTransform, targetRotation);

        if (m_camera != nullptr)
        {
            CameraAPI::setFov(m_camera, m_originalFov);
        }

        finishTransition();
    }
}

bool CameraTransitionController::hasValidStepSequence() const
{
    return m_currentEvent->getTargetPointCount() > 0 && m_currentEvent->getTransitionStep(0) != nullptr;
}

void CameraTransitionController::finishCurrentStepMovement()
{
    Transform* cameraTransform = GameObjectAPI::getTransform(getOwner());

    TransformAPI::setGlobalPosition(cameraTransform, m_stepTargetPosition);
    TransformAPI::setGlobalRotationEuler(cameraTransform, m_stepTargetRotation);

    if (m_camera != nullptr && m_stepUsesFovTransition)
    {
        CameraAPI::setFov(m_camera, m_stepTargetFov);
    }

    CameraTransitionStep* step = m_currentEvent->getTransitionStep(m_currentStepIndex);
    if (step != nullptr)
    {
        step->executeStepReachedActions(this);
    }

    m_state = TransitionState::HoldingStep;
    m_timer = 0.0f;
}

void CameraTransitionController::finishCurrentStepHold()
{
    CameraTransitionStep* step = m_currentEvent->getTransitionStep(m_currentStepIndex);
    if (step != nullptr)
    {
        step->executeStepFinishedActions(this);
    }

    const int nextStepIndex = m_currentStepIndex + 1;

    if (nextStepIndex < m_currentEvent->getTransitionStepCount())
    {
        startStep(nextStepIndex);
        return;
    }

    if (m_currentEvent->isHoldWhileTriggeredMode())
    {
        m_state = TransitionState::WaitingForRelease;
        m_timer = 0.0f;
        return;
    }

    startReturning();
}

Vector3 CameraTransitionController::evaluateStepPosition(float alpha) const
{
    CameraTransitionStep* step = m_currentEvent->getTransitionStep(m_currentStepIndex);
    if (step == nullptr)
    {
        return m_stepTargetPosition;
    }

    switch (step->getMoveMode())
    {
    case CameraStepMoveMode::Linear:
        return MathAPI::lerp(m_stepStartPosition, m_stepTargetPosition, alpha);

    case CameraStepMoveMode::Smooth:
        return MathAPI::lerp(m_stepStartPosition, m_stepTargetPosition, MathAPI::smoothStep(0.0f, 1.0f, alpha));

    case CameraStepMoveMode::CatmullRom:
        return evaluateCatmullRomStepPosition(alpha);

    default:
        return MathAPI::lerp(m_stepStartPosition, m_stepTargetPosition, alpha);
    }
}

Vector3 CameraTransitionController::evaluateStepRotation(float alpha) const
{
    CameraTransitionStep* step = m_currentEvent->getTransitionStep(m_currentStepIndex);
    if (step == nullptr)
    {
        return m_stepTargetRotation;
    }

    switch (step->getMoveMode())
    {
    case CameraStepMoveMode::Linear:
        return MathAPI::lerp(m_stepStartRotation, m_stepTargetRotation, alpha);

    case CameraStepMoveMode::Smooth:
    case CameraStepMoveMode::CatmullRom:
        return MathAPI::lerp(m_stepStartRotation, m_stepTargetRotation, MathAPI::smoothStep(0.0f, 1.0f, alpha));

    default:
        return MathAPI::lerp(m_stepStartRotation, m_stepTargetRotation, alpha);
    }
}

Vector3 CameraTransitionController::evaluateCatmullRomStepPosition(float alpha) const
{
    if (m_currentEvent == nullptr)
    {
        return m_stepTargetPosition;
    }

    const int previousStepIndex = m_currentStepIndex - 1;
    const int nextStepIndex = m_currentStepIndex + 1;

    Vector3 p0 = m_stepStartPosition;
    Vector3 p1 = m_stepStartPosition;
    Vector3 p2 = m_stepTargetPosition;
    Vector3 p3 = m_stepTargetPosition;

    if (previousStepIndex >= 0)
    {
        Transform* previousPoint = m_currentEvent->getTargetPoint(previousStepIndex);
        if (previousPoint != nullptr)
        {
            p0 = TransformAPI::getGlobalPosition(previousPoint);
        }
    }

    Transform* nextPoint = m_currentEvent->getTargetPoint(nextStepIndex);
    if (nextPoint != nullptr)
    {
        p3 = TransformAPI::getGlobalPosition(nextPoint);
    }

    return MathAPI::catmullRom(p0, p1, p2, p3, alpha);
}

void CameraTransitionController::finishTransition()
{
    if (m_cameraFollow != nullptr)
    {
        m_cameraFollow->setFollowEnabled(true);
    }

    if (m_currentEvent->shouldLockGameplayInput())
    {
        setPlayersGameplayInputLocked(false);
    }

    if (m_currentEvent->shouldMakePlayersInvulnerable())
    {
        setPlayersInvulnerable(false);
    }

    if (m_currentEvent->shouldFadeHud() && m_hudFader != nullptr)
    {
        m_hudFader->fadeTo(1.0f, m_hudFadeInDuration);
    }

    m_currentEvent = nullptr;
    m_state = TransitionState::None;
    m_isTransitioning = false;
    m_timer = 0.0f;
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

bool CameraTransitionController::getCameraFollowReturnTarget(Vector3& outPosition, Vector3& outRotation)
{
    if (m_cameraFollow == nullptr)
    {
        return false;
    }

    return m_cameraFollow->getDesiredCameraTransform(outPosition, outRotation);
}

IMPLEMENT_SCRIPT(CameraTransitionController)