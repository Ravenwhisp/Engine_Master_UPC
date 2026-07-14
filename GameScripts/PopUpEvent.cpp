#include "pch.h"
#include "PopUpEvent.h"

#include "GameplayEventTrigger.h"
#include "PopUpController.h"

static const char* popUpTransitionTypeNames[] =
{
    "Fade",
    "Slide From Left",
    "Slide From Right"
};

constexpr int popUpTransitionTypeCount = 3;

static const char* popUpCloseModeNames[] =
{
    "Both Players Confirm",
    "Objective Completed"
};

constexpr int popUpCloseModeCount = 2;

IMPLEMENT_SCRIPT_FIELDS(PopUpEvent,
    SERIALIZED_COMPONENT_REF_VECTOR(m_popUpImages, "PopUp Images", ComponentType::TRANSFORM2D),
    SERIALIZED_ENUM_INT(m_transitionType, "Transition Type", popUpTransitionTypeNames, popUpTransitionTypeCount),
    SERIALIZED_ENUM_INT(m_closeMode, "Close Mode", popUpCloseModeNames, popUpCloseModeCount),
    SERIALIZED_BOOL(m_lockGameplay, "Lock Gameplay"),
    SERIALIZED_FLOAT(m_showDuration, "Show Duration", 0.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_hideDuration, "Hide Duration", 0.0f, 5.0f, 0.05f)
)

PopUpEvent::PopUpEvent(GameObject* owner)
    : GameplayEventAction(owner)
{
}

void PopUpEvent::executeEvent(GameplayEventTrigger* trigger)
{
    if (getPopUpImageCount() == 0)
    {
        Debug::warn("TPopUpEvent on '%s' has no PopUp Images assigned.", GameObjectAPI::getName(getOwner()));
        return;
    }

    if (getPopUpImageTransform2D(0) == nullptr)
    {
        Debug::warn("PopUpEvent on '%s' has an invalid first PopUp Image.", GameObjectAPI::getName(getOwner()));
        return;
    }

    PopUpController* popUpController = findPopUpController();

    if (popUpController == nullptr)
    {
        Debug::warn("PopUpEvent on '%s' could not find PopUpController in the scene.", GameObjectAPI::getName(getOwner()));
        return;
    }

    popUpController->startPopUp(this);
}

Transform2D* PopUpEvent::getPopUpImageTransform2D(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_popUpImages.size()))
    {
        return nullptr;
    }

    return m_popUpImages[index].getReferencedComponent();
}

int PopUpEvent::getPopUpImageCount() const
{
    return static_cast<int>(m_popUpImages.size());
}

PopUpController* PopUpEvent::findPopUpController() const
{
    const std::vector<GameObject*> popUpControllerObjects = SceneAPI::findAllGameObjectsWithScript<PopUpController>();

    if (popUpControllerObjects.empty())
    {
        return nullptr;
    }

    return GameObjectAPI::findScript<PopUpController>(popUpControllerObjects[0]);
}

IMPLEMENT_SCRIPT(PopUpEvent)