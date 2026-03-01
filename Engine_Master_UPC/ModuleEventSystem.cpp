#include "Globals.h"
#include "ModuleEventSystem.h"

#include "Application.h"
#include "InputModule.h"
#include "SceneModule.h"

#include "GameObject.h"
#include "Transform.h"
#include "Transform2D.h"
#include "Canvas.h"
#include "Component.h"

#include <IPointerEventHandler.h>
#include <UIImage.h>


static Vector2 GetMouseScreenPos()
{
    return app->getInputModule()->getMousePosition();
}

static bool IsMouseButtonPressed(PointerButton btn)
{
    InputModule* input = app->getInputModule();
    switch (btn)
    {
    case PointerButton::Left:   return input->isLeftMousePressed();
    case PointerButton::Right:  return input->isRightMousePressed();
    case PointerButton::Middle: return input->isMiddleMousePressed();
    default:                    return false;
    }
}

static bool IsMouseButtonReleased(PointerButton btn)
{
    InputModule* input = app->getInputModule();
    switch (btn)
    {
    case PointerButton::Left:   return input->isLeftMouseReleased();
    case PointerButton::Right:  return input->isRightMouseReleased();
    case PointerButton::Middle: return input->isMiddleMouseReleased();
    default:                    return false;
    }
}

bool ModuleEventSystem::init()
{
    return true;
}

void ModuleEventSystem::update()
{
    process();
}

bool ModuleEventSystem::cleanUp()
{
    for (auto& state : m_buttonStates)
        state = ButtonState{};
    return true;
}

void ModuleEventSystem::process()
{
    const Vector2 mousePos = GetMouseScreenPos();

    // Find the topmost UI element under the cursor
    GameObject* hovered = raycast(mousePos);

    constexpr PointerButton buttons[] = {
        PointerButton::Left,
        PointerButton::Right,
        PointerButton::Middle
    };

    for (PointerButton btn : buttons)
    {
        const int    idx = static_cast<int>(btn);
        ButtonState& state = m_buttonStates[idx];

        PointerEventData data;
        data.button = btn;
        data.position = mousePos;
        data.pointerEnter = hovered;

       
        if (IsMouseButtonPressed(btn))
        {
            state.pointerPress = hovered;
            state.pressPosition = mousePos;

            if (hovered)
            {
                data.pointerPress = hovered;
                data.pressPosition = mousePos;
            }
        }

        if (IsMouseButtonReleased(btn))
        {
            data.pressPosition = state.pressPosition;
            data.pointerPress = state.pointerPress;

            if (state.pointerPress)
            {
                sendPointerUp(state.pointerPress, data);

                // Click = Down and Up landing on the same object
                if (state.pointerPress == hovered)
                {
                    data.pointerClick = hovered;
                    sendPointerClick(hovered, data);
                }
            }

            state.pointerPress = nullptr;
        }
    }
}


GameObject* ModuleEventSystem::raycast(const Vector2& screenPos)
{
    GameObject* best = nullptr;
    int         bestDepth = -1;

    for (GameObject* root : app->getSceneModule()->getAllGameObjects())
    {
        if (!root || !root->GetActive()) continue;

        Canvas* canvas = root->GetComponentAs<Canvas>(ComponentType::CANVAS);
        if (!canvas || !canvas->isActive()) continue;

        raycastAll(root, screenPos, best, bestDepth, 0);
    }
    return best;
}

void ModuleEventSystem::raycastAll(GameObject* go, const Vector2& screenPos, GameObject*& best, int& bestDepth, int depth)
{
    if (!go || !go->GetActive()) return;

    Transform2D* t2d = go->GetComponentAs<Transform2D>(ComponentType::TRANSFORM2D);
    if (t2d && t2d->isActive() && t2d->getRect().contains(screenPos))
    {
        const Rect2D rect = t2d->getRect();

        // Step 1 – rect AABB
        if (rect.contains(screenPos))
        {
            // Step 2 – image bounds refinement (only if a UIImage is present)
            UIImage* img = go->GetComponentAs<UIImage>(ComponentType::UIIMAGE);
            const bool imageHit = !img || !img->isActive() || img->containsPoint(rect, screenPos);

            if (imageHit && depth >= bestDepth)
            {
                best = go;
                bestDepth = depth;
            }
        }
    }

    for (GameObject* child : go->GetTransform()->getAllChildren())
    {
        raycastAll(child, screenPos, best, bestDepth, depth + 1);
    }
}


void ModuleEventSystem::sendPointerClick(GameObject* go, PointerEventData& data)
{
    for (Component* c : go->GetComponents())
    {
        if (auto* h = dynamic_cast<IPointerEventHandler*>(c))
        {
            h->onPointerClick(data);
        }
    }
}

void ModuleEventSystem::sendPointerUp(GameObject* go, PointerEventData& data)
{
    for (Component* c : go->GetComponents())
    {
        if (auto* h = dynamic_cast<IPointerEventHandler*>(c)) h->onPointerUp(data);
    }
 
}