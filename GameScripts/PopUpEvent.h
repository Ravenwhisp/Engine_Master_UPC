#pragma once

#include "ScriptAPI.h"
#include "GameplayEventAction.h"

#include "Transform2D.h"

class GameplayEventTrigger;
class PopUpController;

enum class PopUpTransitionType
{
    Fade = 0,
    SlideFromLeft,
    SlideFromRight
};

enum class PopUpCloseMode
{
    BothPlayersConfirm = 0,
    ObjectiveCompleted
};

class PopUpEvent : public GameplayEventAction
{
    DECLARE_SCRIPT(PopUpEvent)

public:
    explicit PopUpEvent(GameObject* owner);

    void executeEvent(GameplayEventTrigger* trigger) override;

    FieldList getExposedFields() const override;

    Transform2D* getPopUpImageTransform2D(int index) const;
    int getPopUpImageCount() const;

    Transform2D* getPlayer1NotConfirmedTransform2D() const;
    Transform2D* getPlayer1ConfirmedTransform2D() const;
    Transform2D* getPlayer2NotConfirmedTransform2D() const;
    Transform2D* getPlayer2ConfirmedTransform2D() const;

    PopUpTransitionType getTransitionType() const { return static_cast<PopUpTransitionType>(m_transitionType); }
    PopUpCloseMode getCloseMode() const { return static_cast<PopUpCloseMode>(m_closeMode); }

    float getShowDuration() const { return m_showDuration; }
    float getHideDuration() const { return m_hideDuration; }

    bool shouldLockGameplay() const { return m_lockGameplay; }
    bool shouldFadeHud() const { return m_fadeHud; }

private:
    PopUpController* findPopUpController() const;

public:
    std::vector<ComponentRef<Transform2D>> m_popUpImages;

    ComponentRef<Transform2D> m_player1NotConfirmedIndicator;
    ComponentRef<Transform2D> m_player1ConfirmedIndicator;
    ComponentRef<Transform2D> m_player2NotConfirmedIndicator;
    ComponentRef<Transform2D> m_player2ConfirmedIndicator;

    int m_transitionType = static_cast<int>(PopUpTransitionType::Fade);
    int m_closeMode = static_cast<int>(PopUpCloseMode::BothPlayersConfirm);

    float m_showDuration = 0.25f;
    float m_hideDuration = 0.25f;

    bool m_lockGameplay = true;
    bool m_fadeHud = true;
};