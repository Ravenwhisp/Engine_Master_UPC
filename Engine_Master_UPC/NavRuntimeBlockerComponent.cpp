#include "Globals.h"
#include "NavRuntimeBlockerComponent.h"

#include "Application.h"
#include "ModuleNavigation.h"
#include "GameObject.h"
#include "Transform.h"

NavRuntimeBlockerComponent::NavRuntimeBlockerComponent(UID id, GameObject* owner)
	: Component(id, ComponentType::NAV_RUNTIME_BLOCKER, owner)
{
}

std::unique_ptr<Component> NavRuntimeBlockerComponent::clone(GameObject* newOwner) const
{

}