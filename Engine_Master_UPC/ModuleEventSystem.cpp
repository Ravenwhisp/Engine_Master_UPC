#include "Globals.h"
#include "ModuleEventSystem.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleScene.h"
#include "ModuleEditor.h"
#include "WindowGame.h"
#include "ModuleD3D12.h"

#include "GameObject.h"
#include "Transform.h"
#include "Transform2D.h"
#include "Canvas.h"
#include "Component.h"

#include <IPointerEventHandler.h>
#include <UIImage.h>
#include <WindowSceneEditor.h>
#include "Delegates.h"

unsigned int DelegateHandle::CURRENT_ID = 0;


static Vector2 GetMouseScreenPos()
{
    return app->getModuleInput()->getMousePosition();
}

static bool IsMouseButtonPressed(PointerButton btn)
{
    ModuleInput* input = app->getModuleInput();
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
    ModuleInput* input = app->getModuleInput();
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
    if (app->getModuleScene()->isPendingSceneLoad()) {
        clearHoverState();
        return;

    }
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

    constexpr PointerButton buttons[] = 
    {
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

                state.pointerPress = nullptr;
            }
        }
    }
}


bool ModuleEventSystem::getViewportMousePos(Vector2& outPos) const
{
#ifdef GAME_RELEASE

    auto viewport = app->getModuleD3D12()->getSwapChain()->getViewport();

    const ImVec2 size(viewport.Width, viewport.Height);
    const float winX = viewport.TopLeftX;
    const float winY = viewport.TopLeftY;

#else
    auto viewport = app->getModuleEditor()->getEventViewport();
    auto size = app->getModuleEditor()->getEventViewportSize();
    const float winX = viewport.x;
    const float winY = viewport.y;

#endif // GAME_RELEASE

    // Raw mouse position in screen pixels
    const Vector2 rawMouse = app->getModuleInput()->getMousePosition();
    
    // Viewport top-left in screen pixels
    
    const float winW = size.x;
    const float winH = size.y;

    if (winW <= 0.0f || winH <= 0.0f)
    {
        return false;
    }

    // Convert to viewport-local pixels — same space as Rect2D
    const float localX = rawMouse.x - winX;
    const float localY = rawMouse.y - winY;

    // Reject if outside the viewport bounds
    if (localX < 0.0f || localX > winW || localY < 0.0f || localY > winH)
    {
        return false;
    }

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

#ifdef GAME_RELEASE

    auto viewport = app->getModuleD3D12()->getSwapChain()->getViewport();

    const ImVec2 size(viewport.Width, viewport.Height);

#else

    auto size = app->getModuleEditor()->getEventViewportSize();

#endif // GAME_RELEASE

    Rect2D screenRect;
    screenRect.x = 0.0f;
    screenRect.y = 0.0f;
    screenRect.w = size.x;
    screenRect.h = size.y;

    for (GameObject* root : app->getModuleScene()->getAllGameObjects())
    {
        if (!root || !root->GetActive()) continue;

        Canvas* canvas = root->GetComponentAs<Canvas>(ComponentType::CANVAS);
        if (!canvas || !canvas->isActive()) continue;

        raycastAll(root, screenPos, screenRect, best, bestDepth, 0);
    }
    return best;
}

void ModuleEventSystem::raycastAll(GameObject* go, const Vector2& screenPos, const Rect2D& parentRect, GameObject*& best, int& bestDepth, int depth)
{
    if (!go || !go->GetActive()) return;

    Rect2D myRect = parentRect;

    Transform2D* t2d = go->GetComponentAs<Transform2D>(ComponentType::TRANSFORM2D);
    if (t2d && t2d->isActive())
    {
        myRect = t2d->getRect(parentRect);

        if (myRect.contains(screenPos))
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
        raycastAll(child, screenPos, myRect, best, bestDepth, depth + 1);
    }
}


void ModuleEventSystem::sendPointerClick(GameObject* go, PointerEventData& data)
{
    if (!go)
    {
        return;
    }

    for (Component* c : go->GetAllComponents())
    {
        if (auto* h = dynamic_cast<IPointerEventHandler*>(c))
        {
            h->onPointerClick(data);
        }
    }
}

void ModuleEventSystem::sendPointerUp(GameObject* go, PointerEventData& data)
{
    if(!go)
    {
        return;
	}

    for (Component* c : go->GetAllComponents())
    {
        if (auto* h = dynamic_cast<IPointerEventHandler*>(c)) h->onPointerUp(data);
    }
 
}