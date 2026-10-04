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

GameObject* UINavigation::getSelected() const
{
    if (!isValidUID(m_selectedUid))
    {
        return nullptr;
    }

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

    return scene->findGameObjectByUID(m_selectedUid);
}

bool UINavigation::isSelectable(GameObject* go) const
{
    if (!go)
    {
        return false;
    }

    ModuleScene* moduleScene = app->getModuleScene();
    if (!moduleScene || moduleScene->isPendingSceneLoad())
    {
        return false;
    }

    Scene* scene = moduleScene->getScene();
    if (!scene || !scene->containsGameObject(go))
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
    GameObject* selected = getSelected();

    // Clear the handle before invoking component callbacks. This keeps the
    // navigation state valid even if the callback changes or unloads a scene.
    m_selectedUid = INVALID_UID;

    if (!selected)
    {
        return;
    }

    UIButton* btn = selected->GetComponentAs<UIButton>(ComponentType::UIBUTTON);

    if (btn)
    {
        btn->onDeselect();
    }
}

void UINavigation::setSelected(GameObject* go)
{
    if (go && go == getSelected())
    {
        return;
    }

    clearSelection();

    if (!isSelectable(go))
    {
        return;
    }

    m_selectedUid = go->GetID();

    UIButton* btn = go->GetComponentAs<UIButton>(ComponentType::UIBUTTON);

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

    GameObject* selected = getSelected();

    if (!isSelectable(selected))
    {
        clearSelection();

        GameObject* first = findFirstSelectableButton();

        if (first)
        {
            setSelected(first);
        }

        selected = getSelected();
    }

    if (!selected)
    {
        return;
    }

    UIButton* btn =
        selected->GetComponentAs<UIButton>(ComponentType::UIBUTTON);

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
        GameObject* submitTarget = getSelected();
        if (!isSelectable(submitTarget))
        {
            clearSelection();
            return;
        }

        PointerEventData data;
        data.pointerPress = submitTarget;

        ModuleEventSystem* eventSystem = app->getModuleEventSystem();
        if (eventSystem)
        {
            eventSystem->onSubmit(submitTarget, data);
        }
    }
}
