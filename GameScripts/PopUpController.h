#pragma once

#include "ScriptAPI.h"

class PopUpEvent;
class PlayerController;
class Damageable;
class Transform2D;

class PopUpController : public Script
{
    DECLARE_SCRIPT(PopUpController)

public:
    explicit PopUpController(GameObject* owner);

    void Start() override;
    void Update() override;

    void startPopUp(PopUpEvent* event);
    void notifyObjectiveCompleted(GameObject* sourceObject);

    bool isShowingPopUp() const { return !m_activePopUps.empty(); }

private:
    enum class PopUpState
    {
        None,
        Showing,
        Waiting,
        Hiding
    };

    struct ActivePopUp
    {
        PopUpEvent* event = nullptr;
        GameObject* sourceObject = nullptr;

        Transform2D* currentImage = nullptr;
        int currentImageIndex = 0;

        PopUpState state = PopUpState::None;

        bool player1Confirmed = false;
        bool player2Confirmed = false;
        bool objectiveCompleted = false;

        float timer = 0.0f;
        float currentAlpha = 0.0f;

        Vector2 visiblePosition = Vector2(0.0f, 0.0f);
        Vector2 hiddenPosition = Vector2(0.0f, 0.0f);
    };

private:
    void updatePopUp(ActivePopUp& popUp, float dt);
    void removeFinishedPopUps();

    void updateShowing(ActivePopUp& popUp, float dt);
    void updateWaiting(ActivePopUp& popUp);
    void updateHiding(ActivePopUp& popUp, float dt);

    void prepareShowTransition(ActivePopUp& popUp);
    void prepareHideTransition(ActivePopUp& popUp);

    void updateShowTransition(ActivePopUp& popUp, float alpha);
    void updateHideTransition(ActivePopUp& popUp, float alpha);

    bool setCurrentPopUpImage(ActivePopUp& popUp, int index);
    void hideAllPopUpImages(ActivePopUp& popUp);

    void finishPopUp(ActivePopUp& popUp);

    void findPlayerControllers();
    void setPlayersGameplayInputLocked(bool locked);
    void setPlayersInvulnerable(bool invulnerable);

    void setPopUpAlpha(ActivePopUp& popUp, float alpha);
    void setPopUpPosition(ActivePopUp& popUp, const Vector2& position);
    Vector2 calculateHiddenPosition(const ActivePopUp& popUp) const;

private:
    std::vector<ActivePopUp> m_activePopUps;

    std::vector<PlayerController*> m_playerControllers;
    std::vector<Damageable*> m_playerDamageables;

    float m_slideOffset = 600.0f;
};