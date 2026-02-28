#pragma once
#include "Module.h"
#include "PointerEventData.h"
#include <unordered_map>

class GameObject;
class Transform2D;

class EventSystem : public Module
{
public:
    bool init()    override;
    void update()  override;
    bool cleanUp() override;

    void process();

private:

    GameObject* raycast(const Vector2& screenPos);

    void raycastAll(GameObject* root, const Vector2& screenPos, GameObject*& best,int& bestDepth, int depth);

    void sendPointerClick(GameObject* go, PointerEventData& data);
    void sendPointerUp(GameObject* go, PointerEventData& data);

    struct ButtonState
    {
        GameObject*     pointerPress = nullptr;
        Vector2         pressPosition = { 0,0 };
        bool            wasDown = false;
    };

    ButtonState m_buttonStates[3];
};