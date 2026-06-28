#include "pch.h"
#include "Pickup.h"


Pickup::Pickup(GameObject* owner)
    : Script(owner)
{
}

void Pickup::Start()
{
    m_startPosition = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner()));
}

void Pickup::Update()
{
}

void Pickup::OnTriggerEnter(GameObject* gameObject)
{
	m_collected = true;

    GameObjectAPI::removeGameObject(getOwner());
}

IMPLEMENT_SCRIPT(Pickup)