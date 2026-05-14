#pragma once
#include "Module.h"
#include "PointerEventData.h"
#include <unordered_map>
#include "UIRect.h"

class GameObject;
class Transform2D;

class ModuleEventSystem : public Module
{
public:
    bool init()    override;
    void update()  override;
    bool cleanUp() override;

    void process();
    void clearHoverState();

	void setSelected(GameObject* go);
	GameObject* getSelected() const { return m_selected; }

private:
    bool getViewportMousePos(Vector2& outPos) const;
    bool isValidEventTarget(GameObject* go) const;
    GameObject* raycast(const Vector2& screenPos);
    GameObject* m_hoveredLast = nullptr;

    void raycastAll(GameObject* root, const Vector2& screenPos, const Rect2D& parentRect, GameObject*& best, int& bestDepth, int depth);

    void sendPointerEnter(GameObject* go, PointerEventData& data);
    void sendPointerExit(GameObject* go, PointerEventData& data);
    void sendPointerDown(GameObject* go, PointerEventData& data);
    void sendPointerUp(GameObject* go, PointerEventData& data);
    void sendPointerClick(GameObject* go, PointerEventData& data);

	void processNavigation();
	void deselectCurrent();
	void selectGameObject(GameObject* go);
	bool isSelectable(GameObject* go) const;
	GameObject* findFirstSelectableButton() const;

    struct ButtonState
    {
        GameObject*     pointerPress = nullptr;
        Vector2         pressPosition = { 0,0 };
    };

    ButtonState m_buttonStates[3];
	GameObject* m_selected = nullptr;
};