#include "Globals.h"
#include "TriggerComponent.h"

#include "GameObject.h"
#include "Transform.h"

TriggerComponent::TriggerComponent(UID id, GameObject* gameObject)
    : Component(id, ComponentType::TRIGGER, gameObject)
{
}