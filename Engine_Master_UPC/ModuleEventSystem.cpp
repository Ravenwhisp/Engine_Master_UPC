#include "Globals.h"
#include "ModuleEventSystem.h"

#include "Application.h"
#include "InputModule.h"
#include "SceneModule.h"
#include "EditorModule.h"

#include "GameObject.h"
#include "Transform.h"
#include "Transform2D.h"
#include "Canvas.h"
#include "Component.h"

#include <IPointerEventHandler.h>
#include <UIImage.h>
#include <SceneEditor.h>


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
    Vector2 mousePos;
    if (!getViewportMousePos(mousePos))
    {
        clearHoverState();
        return;
    }

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

        if (hovered != state.pointerEnterLast)
        {
            if (state.pointerEnterLast)
            {
                data.pointerEnter = state.pointerEnterLast;
                data.pointerEnter = hovered;
            }

            state.pointerEnterLast = hovered;
        }

       
        if (IsMouseButtonPressed(btn) && hovered)
        {
            state.pointerPress = hovered;
            state.pressPosition = mousePos;

            data.pointerPress = hovered;
            data.pressPosition = mousePos;
        }

        if (IsMouseButtonReleased(btn) && state.pointerPress)
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
        }
    }
}


bool ModuleEventSystem::getViewportMousePos(Vector2& outPos) const
{
    SceneEditor* sceneEditor = app->getEditorModule()->getSceneEditor();
    if (!sceneEditor || !sceneEditor->isHovered())
        return false;

    // Raw mouse position in screen pixels
    const Vector2 rawMouse = app->getInputModule()->getMousePosition();

    // Viewport top-left in screen pixels
    const float winX = sceneEditor->getViewportX();
    const float winY = sceneEditor->getViewportY();
    const float winW = sceneEditor->getSize().x;
    const float winH = sceneEditor->getSize().y;

    if (winW <= 0.0f || winH <= 0.0f)
        return false;

    // Convert to viewport-local pixels — same space as Rect2D
    const float localX = rawMouse.x - winX;
    const float localY = rawMouse.y - winY;

    // Reject if outside the viewport bounds
    if (localX < 0.0f || localX > winW || localY < 0.0f || localY > winH)
        return false;

    outPos = { localX, localY };
    return true;
}
void ModuleEventSystem::clearHoverState()
{
    for (int idx = 0; idx < 3; ++idx)
    {
        ButtonState& state = m_buttonStates[idx];

        if (state.pointerEnterLast)
        {
            PointerEventData data;
            data.button = static_cast<PointerButton>(idx);
            data.pointerEnter = nullptr;
            state.pointerEnterLast = nullptr;
        }

        // Cancel any in-progress press so no ghost click fires on re-entry
        state.pointerPress = nullptr;
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
    if (t2d && t2d->isActive())
    {
        const Rect2D rect = t2d->getRect();

        if (rect.contains(screenPos))
        {
            if (depth >= bestDepth)
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