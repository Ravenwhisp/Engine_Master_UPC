#pragma once
#include "Module.h"
#include "PointerEventData.h"
#include <unordered_map>

class GameObject;
class Transform2D;

class ModuleEventSystem : public Module
{
public:
    bool init()    override;
    void update()  override;
    bool cleanUp() override;

    void process();

private:
    bool getViewportMousePos(Vector2& outPos) const;
    void clearHoverState();
    GameObject* raycast(const Vector2& screenPos);

    void raycastAll(GameObject* root, const Vector2& screenPos, GameObject*& best,int& bestDepth, int depth);

    void sendPointerClick(GameObject* go, PointerEventData& data);
    void sendPointerUp(GameObject* go, PointerEventData& data);

    struct ButtonState
    {
        GameObject*     pointerEnterLast = nullptr;
        GameObject*     pointerPress = nullptr;
        Vector2         pressPosition = { 0,0 };
    };

    ButtonState m_buttonStates[3];
};