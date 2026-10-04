#include "pch.h"
#include "TriggerFire.h"

IMPLEMENT_SCRIPT_FIELDS(TriggerFire,
	SERIALIZED_COMPONENT_REF(m_fireEffectT, "Fire Effect", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_lightT, "Light", ComponentType::TRANSFORM)
)

TriggerFire::TriggerFire(GameObject* owner)
	: Script(owner)
{
}

void TriggerFire::Start()
{
	m_fireTriggered = false;

	Transform* fireEffectTransform = m_fireEffectT.getReferencedComponent();
	if(fireEffectTransform != nullptr)
	{
		GameObject* fireEffectGO = ComponentAPI::getOwner(fireEffectTransform);
		GameObjectAPI::setActive(fireEffectGO, false);
	}

	Transform* lightTransform = m_lightT.getReferencedComponent();
	if(lightTransform != nullptr)
	{
		GameObject* lightGO = ComponentAPI::getOwner(lightTransform);
		GameObjectAPI::setActive(lightGO, false);
	}
}

void TriggerFire::OnTriggerEnter(GameObject* gameObject)
{
	if (gameObject == nullptr || GameObjectAPI::getTag(gameObject) != Tag::PLAYER)
	{
		Debug::log("TriggerFire: Non-player object entered trigger, ignoring.");
		return;
	}
	if (!m_fireTriggered)
	{
		triggerFire();
	}
	else
	{
		Debug::log("TriggerFire: Fire already triggered, ignoring trigger.");
		return;
	}
}

void TriggerFire::triggerFire()
{
	m_fireTriggered = true;

	Transform* fireEffectTransform = m_fireEffectT.getReferencedComponent();
	if(fireEffectTransform != nullptr)
	{
		GameObject* fireEffectGO = ComponentAPI::getOwner(fireEffectTransform);
		GameObjectAPI::setActive(fireEffectGO, true);
	}
	Transform* lightTransform = m_lightT.getReferencedComponent();
	if(lightTransform != nullptr)
	{
		GameObject* lightGO = ComponentAPI::getOwner(lightTransform);
		GameObjectAPI::setActive(lightGO, true);
	}
}

IMPLEMENT_SCRIPT(TriggerFire)
