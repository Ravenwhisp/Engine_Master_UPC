#include "pch.h"
#include "LightZoneManager.h"

IMPLEMENT_SCRIPT_FIELDS(LightZoneManager,
	SERIALIZED_COMPONENT_REF(InteriorLight, "Interior Light"),
	SERIALIZED_COMPONENT_REF(ExteriorLight, "Exterior Light")
)

LightZoneManager::LightZoneManager(GameObject* owner)
    : Script(owner)
{
}

void LightZoneManager::Start()
{
	m_interiorLight = ComponentAPI::getOwner(InteriorLight.getReferencedComponent());
	m_exteriorLight = ComponentAPI::getOwner(ExteriorLight.getReferencedComponent());

    m_isExterior = false;
    applyLightState();
}

void LightZoneManager::Update()
{
}

void LightZoneManager::onBothPlayersCrossed()
{
    m_isExterior = !m_isExterior;
    applyLightState();
    Debug::log("LightZoneManager: Light toggled, exterior=%d", m_isExterior);
}

void LightZoneManager::applyLightState()
{
    if (m_interiorLight) GameObjectAPI::setActive(m_interiorLight, !m_isExterior);
    if (m_exteriorLight) GameObjectAPI::setActive(m_exteriorLight, m_isExterior);
}

IMPLEMENT_SCRIPT(LightZoneManager)
