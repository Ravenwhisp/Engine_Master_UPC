#include "Globals.h"
#include "ModuleEventSystem.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleScene.h"
#include "ModuleEditor.h"
#include "WindowGame.h"
#include "ModuleD3D12.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "Transform2D.h"
#include "Canvas.h"
#include "Component.h"

#include <IPointerEventHandler.h>
#include <UIImage.h>
#include <UIButton.h>
#include <WindowSceneEditor.h>
#include "Delegates.h"

unsigned int DelegateHandle::CURRENT_ID = 0;

#pragma region Statics
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
#pragma endregion

#pragma region Game Loop
bool ModuleEventSystem::init()
{
    return true;
}

void ModuleEventSystem::update()
{
    /*
    if (app->getCurrentEngineState() != ENGINE_STATE::PLAYING)
    {
        return;
    }
    */

    if (app->getModuleScene()->isPendingSceneLoad())
    {
        clearHoverState();
    }
    else
    {
        process();
    }
}

bool ModuleEventSystem::cleanUp()
{
    for (auto& state : m_buttonStates)
        state = ButtonState{};
    return true;
}
#pragma endregion

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

bool ModuleEventSystem::isValidEventTarget(GameObject* go) const
{
    if (!go)
    {
        return false;
    }

    ModuleScene* moduleScene = app->getModuleScene();
    if (!moduleScene)
    {
        return false;
    }

    if (moduleScene->isPendingSceneLoad())
    {
        return false;
    }

    Scene* scene = moduleScene->getScene();
    if (!scene)
    {
        return false;
    }

    return scene->containsGameObject(go);
}

void ModuleEventSystem::clearHoverState()
{
    m_hoveredLast = nullptr;

    for (ButtonState& state : m_buttonStates)
    {
        state.pointerPress = nullptr;
        state.pressPosition = { 0.0f, 0.0f };
    }

	deselectCurrent();
}

bool ModuleEventSystem::isSelectable(GameObject* go) const
{
	if (!isValidEventTarget(go))
		return false;
	if (!go->IsActiveInWindowHierarchy())
		return false;
	UIButton* btn = go->GetComponentAs<UIButton>(ComponentType::UIBUTTON);
	return btn && btn->isActive();
}

void ModuleEventSystem::deselectCurrent()
{
	if (!isValidEventTarget(m_selected))
	{
		m_selected = nullptr;
		return;
	}

	if (UIButton* btn = m_selected->GetComponentAs<UIButton>(ComponentType::UIBUTTON))
	{
		btn->onDeselect();
	}
	m_selected = nullptr;
}

void ModuleEventSystem::selectGameObject(GameObject* go)
{
	if (go == m_selected)
		return;

	deselectCurrent();

	if (!isSelectable(go))
		return;

	m_selected = go;
	if (UIButton* btn = m_selected->GetComponentAs<UIButton>(ComponentType::UIBUTTON))
	{
		btn->onSelect();
	}
}

void ModuleEventSystem::setSelected(GameObject* go)
{
	selectGameObject(go);
}

void ModuleEventSystem::processNavigation()
{
	if (!isValidEventTarget(m_selected))
	{
		m_selected = nullptr;
	}

	ModuleInput* input = app->getModuleInput();
	if (!input)
    {
        return;
    }

	auto navPressed = [&](Keyboard::Keys key) { return input->isKeyJustPressed(key); };

	const bool up = navPressed(Keyboard::Keys::Up) || input->isGamePadDPadUpJustPressed();
	const bool down = navPressed(Keyboard::Keys::Down) || input->isGamePadDPadDownJustPressed();
	const bool left = navPressed(Keyboard::Keys::Left) || input->isGamePadDPadLeftJustPressed();
	const bool right = navPressed(Keyboard::Keys::Right) || input->isGamePadDPadRightJustPressed();
	const bool submit = navPressed(Keyboard::Keys::Enter) || navPressed(Keyboard::Keys::Space) || input->isGamePadAJustPressed();
	const bool anyNav = up || down || left || right || submit;

    if (!anyNav)
    {
        return;
    }

	if (!m_selected)
	{
		if (GameObject* first = findFirstSelectableButton())
		{
			selectGameObject(first);
		}
	}

    if (!m_selected)
    {
        return;
    }

	UIButton* btn = m_selected->GetComponentAs<UIButton>(ComponentType::UIBUTTON);
	if (!btn)
    {
        return;
    }

	if (up && btn->getNavUp())
    {
        selectGameObject(btn->getNavUp()->getOwner());
    }
	else if (down && btn->getNavDown())
    {
        selectGameObject(btn->getNavDown()->getOwner());
    }
	else if (left && btn->getNavLeft())
    {
        selectGameObject(btn->getNavLeft()->getOwner());
    }
	else if (right && btn->getNavRight())
    {
        selectGameObject(btn->getNavRight()->getOwner());
    }

	if (submit)
	{
        PointerEventData data;
        data.pointerPress = m_selected;
        sendPointerUp(m_selected, data);
		deselectCurrent();
	}

}

GameObject* ModuleEventSystem::findFirstSelectableButton() const
{
    ModuleScene* ms = app->getModuleScene();
    if (!ms)
        return nullptr;
    Scene* scene = ms->getScene();
    if (!scene)
        return nullptr;

    for (GameObject* go : scene->getAllGameObjects())
    {
        if (isSelectable(go))
            return go;
    }

    return nullptr;
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

    for (GameObject* root : app->getModuleScene()->getScene()->getAllGameObjects())
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
        myRect = t2d->getRect(parentRect, { 1.0f, 1.0f });

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

#pragma region Events
void ModuleEventSystem::process()
{
    processNavigation();

    Vector2 mousePos;
    if (!getViewportMousePos(mousePos))
    {
        clearHoverState();
        return;
    }

    GameObject* hovered = raycast(mousePos);

    if (!isValidEventTarget(hovered))
    {
        hovered = nullptr;
    }

    if (!isValidEventTarget(m_hoveredLast))
    {
        m_hoveredLast = nullptr;
    }

    if (hovered != m_hoveredLast)
    {
        PointerEventData data;
        data.position = mousePos;

        if (m_hoveredLast)
        {
            data.pointerEnter = m_hoveredLast;
            sendPointerExit(m_hoveredLast, data);
        }

        if (hovered)
        {
            data.pointerEnter = hovered;
            sendPointerEnter(hovered, data);

			if (isSelectable(hovered))
			{
				selectGameObject(hovered);
			}
        }

        m_hoveredLast = hovered;
    }

    constexpr PointerButton buttons[] =
    {
        PointerButton::Left,
        PointerButton::Right,
        PointerButton::Middle
    };

    for (PointerButton btn : buttons)
    {
        int idx = (int)btn;
        ButtonState& state = m_buttonStates[idx];

        if (!isValidEventTarget(state.pointerPress))
        {
            state.pointerPress = nullptr;
            state.pressPosition = { 0.0f, 0.0f };
        }

        PointerEventData data;
        data.button = btn;
        data.position = mousePos;

        if (IsMouseButtonPressed(btn) && hovered)
        {
            state.pointerPress = hovered;
            state.pressPosition = mousePos;

            data.pointerPress = hovered;
            data.pressPosition = mousePos;

            sendPointerDown(hovered, data);

			if (isSelectable(hovered))
			{
				selectGameObject(hovered);
			}
        }

        if (IsMouseButtonReleased(btn) && state.pointerPress)
        {
            data.pointerPress = state.pointerPress;
            data.pressPosition = state.pressPosition;

            sendPointerUp(state.pointerPress, data);
            deselectCurrent();

            if (state.pointerPress == hovered)
            {
                sendPointerClick(hovered, data);

                if (app->getModuleScene()->isPendingSceneLoad())
                {
                    clearHoverState();
                    return;
                }
            }

            state.pointerPress = nullptr;
            state.pressPosition = { 0.0f, 0.0f };
        }
    }
}
void ModuleEventSystem::sendPointerEnter(GameObject* go, PointerEventData& data)
{
    if (!isValidEventTarget(go))
    {
        return;
    }

    for (Component* c : go->GetAllComponents())
    {
        if (auto* h = dynamic_cast<IPointerEventHandler*>(c))
        {
            h->onPointerEnter(data);
        }
    }
}

void ModuleEventSystem::sendPointerExit(GameObject* go, PointerEventData& data)
{
    if (!isValidEventTarget(go))
    {
        return;
    }

    for (Component* c : go->GetAllComponents())
    {
        if (auto* h = dynamic_cast<IPointerEventHandler*>(c))
        {
            h->onPointerExit(data);
        }
    }
}

void ModuleEventSystem::sendPointerDown(GameObject* go, PointerEventData& data)
{
    if (!isValidEventTarget(go))
    {
        return;
    }

    for (Component* c : go->GetAllComponents())
    {
        if (auto* h = dynamic_cast<IPointerEventHandler*>(c))
        {
            h->onPointerDown(data);
        }
    }
}

void ModuleEventSystem::sendPointerUp(GameObject* go, PointerEventData& data)
{
    if (!isValidEventTarget(go))
    {
        return;
    }

    for (Component* c : go->GetAllComponents())
    {
        if (auto* h = dynamic_cast<IPointerEventHandler*>(c))
        {
            h->onPointerUp(data);
        }
    }
}

void ModuleEventSystem::sendPointerClick(GameObject* go, PointerEventData& data)
{
    if (!isValidEventTarget(go))
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
#pragma endregion
