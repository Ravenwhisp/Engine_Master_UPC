#include "pch.h"
#include "PopUpController.h"

#include "PopUpEvent.h"
#include "PlayerController.h"
#include "Damageable.h"
#include "HUDFader.h"

PopUpController::PopUpController(GameObject* owner)
    : Script(owner)
{
}

void PopUpController::Start()
{
    findPlayerControllers();
    findHUDFader();
}

void PopUpController::Update()
{
    if (m_activePopUps.empty())
    {
        return;
    }

    const float dt = Time::getDeltaTime();

    for (ActivePopUp& popUp : m_activePopUps)
    {
        updatePopUp(popUp, dt);
    }

    removeFinishedPopUps();
}

void PopUpController::startEvent(PopUpEvent* event)
{
    if (event == nullptr)
    {
        return;
    }

    ActivePopUp popUp;
    popUp.event = event;
    popUp.sourceObject = event->getOwner();

    setUpConfirmationIndicators(popUp);
    hideAllPopUpImages(popUp);
    startEventEffects(popUp);

    if (!startPopUp(popUp, 0))
    {
        return;
    }

    m_activePopUps.push_back(popUp);
}

void PopUpController::notifyObjectiveCompleted(GameObject* sourceObject)
{
    if (sourceObject == nullptr)
    {
        return;
    }

    for (ActivePopUp& popUp : m_activePopUps)
    {
        if (popUp.event == nullptr)
        {
            continue;
        }

        if (popUp.sourceObject != sourceObject)
        {
            continue;
        }

        if (popUp.event->getCloseMode() != PopUpCloseMode::ObjectiveCompleted)
        {
            continue;
        }

        popUp.objectiveCompleted = true;
    }
}

bool PopUpController::startPopUp(ActivePopUp& popUp, int imageIndex)
{
    if (popUp.event == nullptr)
    {
        return false;
    }

    popUp.currentImageIndex = imageIndex;
    popUp.state = PopUpState::Showing;

    popUp.timer = 0.0f;
    popUp.currentAlpha = 0.0f;

    popUp.player1Confirmed = false;
    popUp.player2Confirmed = false;
    popUp.objectiveCompleted = false;

    updateConfirmationIndicators(popUp, 0.0f);

    if (!setCurrentPopUpImage(popUp, popUp.currentImageIndex))
    {
        finishEvent(popUp);
        return false;
    }

    prepareShowTransition(popUp);
    return true;
}

void PopUpController::startEventEffects(ActivePopUp& popUp)
{
    if (popUp.event == nullptr)
    {
        return;
    }

    if (popUp.event->shouldLockGameplay())
    {
        setPlayersGameplayInputLocked(true);
        setPlayersInvulnerable(true);
    }

    fadeHudOut(popUp);
}

void PopUpController::updatePopUp(ActivePopUp& popUp, float dt)
{
    switch (popUp.state)
    {
    case PopUpState::Showing:
        updateShowing(popUp, dt);
        break;

    case PopUpState::Waiting:
        updateWaiting(popUp);
        break;

    case PopUpState::Hiding:
        updateHiding(popUp, dt);
        break;

    case PopUpState::None:
    default:
        break;
    }
}

void PopUpController::removeFinishedPopUps()
{
    for (int i = static_cast<int>(m_activePopUps.size()) - 1; i >= 0; --i)
    {
        if (m_activePopUps[i].state == PopUpState::None)
        {
            m_activePopUps.erase(m_activePopUps.begin() + i);
        }
    }
}

void PopUpController::updateShowing(ActivePopUp& popUp, float dt)
{
    const float duration = popUp.event->getShowDuration();

    popUp.timer += dt;

    const float normalizedTime = popUp.timer >= duration ? 1.0f : popUp.timer / duration;
    const float alpha = MathAPI::smoothStep(0.0f, 1.0f, normalizedTime);

    updateShowTransition(popUp, alpha);

    if (popUp.timer >= duration)
    {
        updateShowTransition(popUp, 1.0f);

        popUp.state = PopUpState::Waiting;
        popUp.timer = 0.0f;
    }
}

void PopUpController::updateWaiting(ActivePopUp& popUp)
{
    if (popUp.event == nullptr)
    {
        return;
    }

    switch (popUp.event->getCloseMode())
    {
    case PopUpCloseMode::BothPlayersConfirm:
        if (Input::isFaceButtonBottomJustPressed(0))
        {
            popUp.player1Confirmed = true;
            updateConfirmationIndicators(popUp);
        }

        if (Input::isFaceButtonBottomJustPressed(1))
        {
            popUp.player2Confirmed = true;
            updateConfirmationIndicators(popUp);
        }

        if (popUp.player1Confirmed && popUp.player2Confirmed)
        {
            prepareHideTransition(popUp);
            popUp.state = PopUpState::Hiding;
            popUp.timer = 0.0f;
        }
        break;

    case PopUpCloseMode::ObjectiveCompleted:
        if (popUp.objectiveCompleted)
        {
            prepareHideTransition(popUp);
            popUp.state = PopUpState::Hiding;
            popUp.timer = 0.0f;
        }
        break;

    default:
        break;
    }
}

void PopUpController::updateHiding(ActivePopUp& popUp, float dt)
{
    if (popUp.event == nullptr)
    {
        return;
    }

    const float duration = popUp.event->getHideDuration();

    popUp.timer += dt;

    const float normalizedTime = popUp.timer >= duration ? 1.0f : popUp.timer / duration;
    const float alpha = MathAPI::smoothStep(0.0f, 1.0f, normalizedTime);

    updateHideTransition(popUp, alpha);

    if (popUp.timer < duration)
    {
        return;
    }

    updateHideTransition(popUp, 1.0f);

    const int nextImageIndex = popUp.currentImageIndex + 1;
    const int imageCount = popUp.event->getPopUpImageCount();

    finishPopUp(popUp);

    if (nextImageIndex < imageCount)
    {
        if (startPopUp(popUp, nextImageIndex))
        {
            return;
        }
    }

    finishEvent(popUp);
}

void PopUpController::prepareShowTransition(ActivePopUp& popUp)
{
    if (popUp.currentImage == nullptr || popUp.event == nullptr)
    {
        return;
    }

    popUp.visiblePosition = Transform2DAPI::getPosition(popUp.currentImage);
    popUp.hiddenPosition = calculateHiddenPosition(popUp);

    switch (popUp.event->getTransitionType())
    {
    case PopUpTransitionType::Fade:
        setPopUpAlpha(popUp, 0.0f);
        break;

    case PopUpTransitionType::SlideFromLeft:
    case PopUpTransitionType::SlideFromRight:
        setPopUpAlpha(popUp, 1.0f);
        setPopUpPosition(popUp, popUp.hiddenPosition);
        break;

    default:
        setPopUpAlpha(popUp, 0.0f);
        break;
    }
}

void PopUpController::prepareHideTransition(ActivePopUp& popUp)
{
    if (popUp.currentImage == nullptr || popUp.event == nullptr)
    {
        return;
    }

    popUp.visiblePosition = Transform2DAPI::getPosition(popUp.currentImage);
    popUp.hiddenPosition = calculateHiddenPosition(popUp);

    switch (popUp.event->getTransitionType())
    {
    case PopUpTransitionType::Fade:
        setPopUpAlpha(popUp, 1.0f);
        break;

    case PopUpTransitionType::SlideFromLeft:
    case PopUpTransitionType::SlideFromRight:
        setPopUpAlpha(popUp, 1.0f);
        setPopUpPosition(popUp, popUp.visiblePosition);
        break;

    default:
        setPopUpAlpha(popUp, 1.0f);
        break;
    }
}

void PopUpController::updateShowTransition(ActivePopUp& popUp, float alpha)
{
    if (popUp.event == nullptr || popUp.currentImage == nullptr)
    {
        return;
    }

    switch (popUp.event->getTransitionType())
    {
    case PopUpTransitionType::Fade:
        popUp.currentAlpha = alpha;
        setPopUpAlpha(popUp, popUp.currentAlpha);
        updateConfirmationIndicators(popUp, alpha);
        break;

    case PopUpTransitionType::SlideFromLeft:
    case PopUpTransitionType::SlideFromRight:
    {
        const Vector2 position = MathAPI::lerp(popUp.hiddenPosition, popUp.visiblePosition, alpha);
        setPopUpPosition(popUp, position);
        break;
    }

    default:
        popUp.currentAlpha = alpha;
        setPopUpAlpha(popUp, popUp.currentAlpha);
        break;
    }
}

void PopUpController::updateHideTransition(ActivePopUp& popUp, float alpha)
{
    if (popUp.event == nullptr || popUp.currentImage == nullptr)
    {
        return;
    }

    switch (popUp.event->getTransitionType())
    {
    case PopUpTransitionType::Fade:
        popUp.currentAlpha = MathAPI::lerp(1.0f, 0.0f, alpha);
        setPopUpAlpha(popUp, popUp.currentAlpha);
        updateConfirmationIndicators(popUp, popUp.currentAlpha);
        break;

    case PopUpTransitionType::SlideFromLeft:
    case PopUpTransitionType::SlideFromRight:
    {
        const Vector2 position = MathAPI::lerp(popUp.visiblePosition, popUp.hiddenPosition, alpha);
        setPopUpPosition(popUp, position);
        break;
    }

    default:
        popUp.currentAlpha = MathAPI::lerp(1.0f, 0.0f, alpha);
        setPopUpAlpha(popUp, popUp.currentAlpha);
        break;
    }
}

bool PopUpController::setCurrentPopUpImage(ActivePopUp& popUp, int index)
{
    if (popUp.event == nullptr)
    {
        return false;
    }

    popUp.currentImage = popUp.event->getPopUpImageTransform2D(index);

    if (popUp.currentImage == nullptr)
    {
        Debug::warn("PopUpController could not set PopUp Image at index %d.", index);
        return false;
    }

    return true;
}

void PopUpController::hideAllPopUpImages(ActivePopUp& popUp)
{
    if (popUp.event == nullptr)
    {
        return;
    }

    const int imageCount = popUp.event->getPopUpImageCount();

    for (int i = 0; i < imageCount; ++i)
    {
        Transform2D* image = popUp.event->getPopUpImageTransform2D(i);

        if (image == nullptr)
        {
            continue;
        }

        Transform2DAPI::setAlpha(image, 0.0f);
    }
}

void PopUpController::finishPopUp(ActivePopUp& popUp)
{
    if (popUp.currentImage != nullptr)
    {
        Transform2DAPI::setAlpha(popUp.currentImage, 0.0f);
    }

    popUp.currentImage = nullptr;
}

void PopUpController::finishEvent(ActivePopUp& popUp)
{
    finishEventEffects(popUp);

    popUp.event = nullptr;
    popUp.sourceObject = nullptr;
    popUp.currentImage = nullptr;
    popUp.currentImageIndex = 0;

    popUp.state = PopUpState::None;
}

void PopUpController::finishEventEffects(ActivePopUp& popUp)
{
    if (popUp.event == nullptr)
    {
        return;
    }

    if (popUp.event->shouldLockGameplay())
    {
        setPlayersGameplayInputLocked(false);
        setPlayersInvulnerable(false);
    }

    fadeHudIn(popUp);

    hideConfirmationIndicators(popUp);
}

void PopUpController::findPlayerControllers()
{
    m_playerControllers.clear();
    m_playerDamageables.clear();

    const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER, true);

    for (GameObject* player : players)
    {
        PlayerController* playerController = GameObjectAPI::findScript<PlayerController>(player);
        if (playerController == nullptr)
        {
            Debug::warn("PopUpController could not find PlayerController on player '%s'.", GameObjectAPI::getName(player));
        }
        else
        {
            m_playerControllers.push_back(playerController);
        }

        Damageable* damageable = GameObjectAPI::findScript<Damageable>(player);
        if (damageable == nullptr)
        {
            Debug::warn("PopUpController could not find Damageable on player '%s'.", GameObjectAPI::getName(player));
        }
        else
        {
            m_playerDamageables.push_back(damageable);
        }
    }
}

void PopUpController::setPlayersGameplayInputLocked(bool locked)
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

void PopUpController::setPlayersInvulnerable(bool invulnerable)
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

void PopUpController::findHUDFader()
{
    const std::vector<GameObject*> hudFaderObjects = SceneAPI::findAllGameObjectsWithScript<HUDFader>();

    if (hudFaderObjects.empty())
    {
        Debug::warn("PopUpController could not find any GameObject with HUDFader.");
        return;
    }

    m_hudFader = GameObjectAPI::findScript<HUDFader>(hudFaderObjects[0]);
}

void PopUpController::fadeHudOut(const ActivePopUp& popUp)
{
    if (popUp.event == nullptr)
    {
        return;
    }

    if (!popUp.event->shouldFadeHud())
    {
        return;
    }

    if (m_hudFader == nullptr)
    {
        return;
    }

    m_hudFader->fadeTo(0.0f, m_hudFadeOutDuration);
}

void PopUpController::fadeHudIn(const ActivePopUp& popUp)
{
    if (popUp.event == nullptr)
    {
        return;
    }

    if (!popUp.event->shouldFadeHud())
    {
        return;
    }

    if (m_hudFader == nullptr)
    {
        return;
    }

    m_hudFader->fadeTo(1.0f, m_hudFadeInDuration);
}

void PopUpController::setPopUpAlpha(ActivePopUp& popUp, float alpha)
{
    if (popUp.currentImage == nullptr)
    {
        return;
    }

    Transform2DAPI::setAlpha(popUp.currentImage, alpha);
}

void PopUpController::setPopUpPosition(ActivePopUp& popUp, const Vector2& position)
{
    if (popUp.currentImage == nullptr)
    {
        return;
    }

    Transform2DAPI::setPosition(popUp.currentImage, position);
}

Vector2 PopUpController::calculateHiddenPosition(const ActivePopUp& popUp) const
{
    if (popUp.event == nullptr)
    {
        return popUp.visiblePosition;
    }

    switch (popUp.event->getTransitionType())
    {
    case PopUpTransitionType::SlideFromLeft:
        return Vector2(popUp.visiblePosition.x - m_slideOffset, popUp.visiblePosition.y);

    case PopUpTransitionType::SlideFromRight:
        return Vector2(popUp.visiblePosition.x + m_slideOffset, popUp.visiblePosition.y);

    case PopUpTransitionType::Fade:
    default:
        return popUp.visiblePosition;
    }
}

bool PopUpController::shouldUseConfirmationIndicators(const ActivePopUp& popUp) const
{
    if (popUp.event == nullptr)
    {
        return false;
    }

    return popUp.event->getCloseMode() == PopUpCloseMode::BothPlayersConfirm && popUp.event->getTransitionType() == PopUpTransitionType::Fade;
}

void PopUpController::setUpConfirmationIndicators(ActivePopUp& popUp)
{
    if (popUp.event == nullptr)
    {
        return;
    }

    popUp.player1NotConfirmedIndicator = popUp.event->getPlayer1NotConfirmedTransform2D();
    popUp.player1ConfirmedIndicator = popUp.event->getPlayer1ConfirmedTransform2D();
    popUp.player2NotConfirmedIndicator = popUp.event->getPlayer2NotConfirmedTransform2D();
    popUp.player2ConfirmedIndicator = popUp.event->getPlayer2ConfirmedTransform2D();
}

void PopUpController::updateConfirmationIndicators(ActivePopUp& popUp, float alphaMultiplier)
{
    if (!shouldUseConfirmationIndicators(popUp))
    {
        hideConfirmationIndicators(popUp);
        return;
    }

    setIndicatorAlpha(popUp.player1NotConfirmedIndicator, !popUp.player1Confirmed ? alphaMultiplier : 0.0f);
    setIndicatorAlpha(popUp.player1ConfirmedIndicator, popUp.player1Confirmed ? alphaMultiplier : 0.0f);

    setIndicatorAlpha(popUp.player2NotConfirmedIndicator, !popUp.player2Confirmed ? alphaMultiplier : 0.0f);
    setIndicatorAlpha(popUp.player2ConfirmedIndicator, popUp.player2Confirmed ? alphaMultiplier : 0.0f);
}

void PopUpController::hideConfirmationIndicators(ActivePopUp& popUp)
{
    setIndicatorAlpha(popUp.player1NotConfirmedIndicator, 0.0f);
    setIndicatorAlpha(popUp.player1ConfirmedIndicator, 0.0f);
    setIndicatorAlpha(popUp.player2NotConfirmedIndicator, 0.0f);
    setIndicatorAlpha(popUp.player2ConfirmedIndicator, 0.0f);
}

void PopUpController::setIndicatorAlpha(Transform2D* indicator, float alpha)
{
    if (indicator == nullptr)
    {
        return;
    }

    Transform2DAPI::setAlpha(indicator, alpha);
}

IMPLEMENT_SCRIPT(PopUpController)