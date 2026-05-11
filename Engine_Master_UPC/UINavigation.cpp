#include "Globals.h"
#include "UINavigation.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "Component.h"

#include <UIButton.h>
#include "ModuleEventSystem.h"

void UINavigation::update()
{
    processNavigation();
}

bool UINavigation::isSelectable(GameObject* go) const
{
    if (!go)
    {
        return false;
    }

    if (!go->IsActiveInWindowHierarchy())
    {
        return false;
    }

    UIButton* btn = go->GetComponentAs<UIButton>(ComponentType::UIBUTTON);

    return btn && btn->isActive();
}

void UINavigation::clearSelection()
{
    if (!m_selected)
    {
        return;
    }

    UIButton* btn = m_selected->GetComponentAs<UIButton>(ComponentType::UIBUTTON);

    if (btn)
    {
        btn->onDeselect();
    }

    m_selected = nullptr;
}

void UINavigation::setSelected(GameObject* go)
{
    if (go == m_selected)
    {
        return;
    }

    clearSelection();

    if (!isSelectable(go))
    {
        return;
    }

    m_selected = go;

    UIButton* btn = m_selected->GetComponentAs<UIButton>(ComponentType::UIBUTTON);

    if (btn)
    {
        btn->onSelect();
    }
}

GameObject* UINavigation::findFirstSelectableButton() const
{
    ModuleScene* moduleScene = app->getModuleScene();

    if (!moduleScene)
    {
        return nullptr;
    }

    Scene* scene = moduleScene->getScene();

    if (!scene)
    {
        return nullptr;
    }

    for (GameObject* go : scene->getAllGameObjects())
    {
        if (isSelectable(go))
        {
            return go;
        }
    }

    return nullptr;
}

void UINavigation::processNavigation()
{
    ModuleInput* input = app->getModuleInput();

    if (!input)
    {
        return;
    }

    auto navPressed = [input](Keyboard::Keys key)
    {
        return input->isKeyJustPressed(key);
    };

    const bool up =
        navPressed(Keyboard::Keys::Up) ||
        input->isGamePadDPadUpJustPressed();

    const bool down =
        navPressed(Keyboard::Keys::Down) ||
        input->isGamePadDPadDownJustPressed();

    const bool left =
        navPressed(Keyboard::Keys::Left) ||
        input->isGamePadDPadLeftJustPressed();

    const bool right =
        navPressed(Keyboard::Keys::Right) ||
        input->isGamePadDPadRightJustPressed();

    const bool submit =
        navPressed(Keyboard::Keys::Enter) ||
        input->isGamePadAJustPressed();

    const bool anyNav = up || down || left || right || submit;

    if (!anyNav)
    {
        return;
    }

    if (!m_selected || !isSelectable(m_selected))
    {
        GameObject* first = findFirstSelectableButton();

        if (first)
        {
            setSelected(first);
        }
    }

    if (!m_selected)
    {
        return;
    }

    UIButton* btn =
        m_selected->GetComponentAs<UIButton>(ComponentType::UIBUTTON);

    if (!btn)
    {
        return;
    }

    if (up && btn->getNavUp())
    {
        setSelected(btn->getNavUp()->getOwner());
    }
    else if (down && btn->getNavDown())
    {
        setSelected(btn->getNavDown()->getOwner());
    }
    else if (left && btn->getNavLeft())
    {
        setSelected(btn->getNavLeft()->getOwner());
    }
    else if (right && btn->getNavRight())
    {
        setSelected(btn->getNavRight()->getOwner());
    }

    if (submit)
    {
        PointerEventData data;
        data.pointerPress = m_selected;

        ModuleEventSystem* eventSystem = app->getModuleEventSystem();
        if (eventSystem)
        {
            eventSystem->onSubmit(m_selected, data);
        }
    }
}