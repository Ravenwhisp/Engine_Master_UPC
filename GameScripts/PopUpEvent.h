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

    ScriptFieldList getExposedFields() const override;

    Transform2D* getPopUpImageTransform2D(int index) const;
    int getPopUpImageCount() const;

    PopUpTransitionType getTransitionType() const { return static_cast<PopUpTransitionType>(m_transitionType); }
    PopUpCloseMode getCloseMode() const { return static_cast<PopUpCloseMode>(m_closeMode); }

    float getShowDuration() const { return m_showDuration; }
    float getHideDuration() const { return m_hideDuration; }

    bool shouldLockGameplay() const { return m_lockGameplay; }

private:
    PopUpController* findPopUpController() const;

public:
    std::vector<ScriptComponentRef<Transform2D>> m_popUpImages;

    int m_transitionType = static_cast<int>(PopUpTransitionType::Fade);
    int m_closeMode = static_cast<int>(PopUpCloseMode::BothPlayersConfirm);

    float m_showDuration = 0.25f;
    float m_hideDuration = 0.25f;

    bool m_lockGameplay = true;
};