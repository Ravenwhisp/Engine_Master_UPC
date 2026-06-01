#include "pch.h"
#include "LightZoneManager.h"

IMPLEMENT_SCRIPT_FIELDS(LightZoneManager,
    SERIALIZED_STRING(interiorLightName, "Interior Light Name"),
    SERIALIZED_STRING(exteriorLightName, "Exterior Light Name")
)

LightZoneManager::LightZoneManager(GameObject* owner)
    : Script(owner)
{
}

void LightZoneManager::Start()
{
    auto allObjects = SceneAPI::getAllGameObjectsInScene();
    for (GameObject* obj : allObjects)
    {
        const char* name = GameObjectAPI::getName(obj);
        if (name && strcmp(name, interiorLightName) == 0)
            m_interiorLight = obj;
        else if (name && strcmp(name, exteriorLightName) == 0)
            m_exteriorLight = obj;
    }

    if (!m_interiorLight) Debug::warn("LightZoneManager: Interior light '%s' not found!", interiorLightName);
    if (!m_exteriorLight) Debug::warn("LightZoneManager: Exterior light '%s' not found!", exteriorLightName);

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
