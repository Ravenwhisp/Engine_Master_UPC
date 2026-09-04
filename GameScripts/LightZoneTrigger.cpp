#include "pch.h"
#include "LightZoneTrigger.h"
#include "LightZoneManager.h"

IMPLEMENT_SCRIPT_FIELDS(LightZoneTrigger,
    SERIALIZED_FLOAT(m_xWidth, "Trigger Width X", 0.1f, 100.0f, 0.1f),
    SERIALIZED_FLOAT(m_zWidth, "Trigger Width Z", 0.1f, 100.0f, 0.1f)
)

LightZoneTrigger::LightZoneTrigger(GameObject* owner)
    : Script(owner)
{
}

void LightZoneTrigger::Start()
{
    auto managers = SceneAPI::findAllGameObjectsWithScript<LightZoneManager>();
    for (GameObject* obj : managers)
    {
        m_manager = GameObjectAPI::findScript<LightZoneManager>(obj);
        if (m_manager) break;
    }

    if (!m_manager)
    {
        Debug::warn("LightZoneTrigger: No LightZoneManager found in scene!");
    }

    m_playersInside.clear();
    m_bothWereInside = false;
}

void LightZoneTrigger::Update()
{
    if (!m_manager) return;

    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);
    const Vector3 triggerCenter = TransformAPI::getGlobalPosition(ownerTransform);

    std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    for (GameObject* player : players)
    {
        Transform* pt = GameObjectAPI::getTransform(player);
        const Vector3 pos = TransformAPI::getGlobalPosition(pt);

        if (containsPoint(triggerCenter, pos))
            m_playersInside.insert(player);
        else
            m_playersInside.erase(player);
    }

    bool bothAreInside = !players.empty() && (m_playersInside.size() >= players.size());

    if (bothAreInside && !m_bothWereInside)
    {
		m_manager->onBothPlayersCrossed();
    }

    m_bothWereInside = bothAreInside;
}

bool LightZoneTrigger::containsPoint(const Vector3& center, const Vector3& point) const
{
    const float halfX = m_xWidth * 0.5f;
    const float halfZ = m_zWidth * 0.5f;

    return point.x >= center.x - halfX &&
        point.x <= center.x + halfX &&
        point.z >= center.z - halfZ &&
        point.z <= center.z + halfZ;
}

IMPLEMENT_SCRIPT(LightZoneTrigger)
