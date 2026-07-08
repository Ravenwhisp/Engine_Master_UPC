#pragma once
#include "Module.h"
#include "PointerEventData.h"
#include "UIRect.h"

class GameObject;
class UINavigation;

class ModuleEventSystem : public Module
{
public:
    bool init()    override;
    void update()  override;
    bool cleanUp() override;

    void process();
    void clearHoverState();
    void onSubmit(GameObject* go, PointerEventData& data);

private:
    void processMouse();
    void processController();

    bool getViewportMousePos(Vector2& outPos) const;
    bool isValidEventTarget(GameObject* go) const;

    GameObject* raycast(const Vector2& screenPos);

    void raycastAll(GameObject* root, const Vector2& screenPos, const Rect2D& parentRect, GameObject*& best, int& bestDepth, int depth, const Vector2& inheritedScale);


    void sendPointerEnter(GameObject* go, PointerEventData& data);
    void sendPointerExit(GameObject* go, PointerEventData& data);
    void sendPointerDown(GameObject* go, PointerEventData& data);
    void sendPointerUp(GameObject* go, PointerEventData& data);
    void sendPointerClick(GameObject* go, PointerEventData& data);

private:

    struct ButtonState
    {
        GameObject*     pointerPress = nullptr;
        Vector2         pressPosition = { 0,0 };
    };

    ButtonState m_buttonStates[3];

    GameObject* m_hoveredLast = nullptr;
    UINavigation* m_navigation = nullptr;
};