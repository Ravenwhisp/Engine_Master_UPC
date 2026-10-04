#include "pch.h"
#include "UIMultiplayerSwitcher.h"
#include "PlayerGamepadBinding.h"

UIMultiplayerSwitcher::UIMultiplayerSwitcher(GameObject* owner)
    : Script(owner)
{
}

void UIMultiplayerSwitcher::Start()
{
    Transform* kbComp = TransformAPI::findChildByName(getOwner()->GetTransform(), "kb");
    Transform* gpComp = TransformAPI::findChildByName(getOwner()->GetTransform(), "gp");

    if (!kbComp || !gpComp)
    {
        return;
    }

    if (PlayerGamepadBinding::getGamepadDeviceIndex(0) >= 0)
    {
        GameObjectAPI::setActive(kbComp->getOwner(), false);
        GameObjectAPI::setActive(gpComp->getOwner(), true);
    }
    else
    {
        GameObjectAPI::setActive(kbComp->getOwner(), true);
        GameObjectAPI::setActive(gpComp->getOwner(), false);
    }
}

void UIMultiplayerSwitcher::Update()
{
}

IMPLEMENT_SCRIPT(UIMultiplayerSwitcher)