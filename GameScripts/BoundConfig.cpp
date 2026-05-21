#include "pch.h"
#include "BoundConfig.h"

IMPLEMENT_DATACONTAINER(BoundConfig)

bool BoundConfig::deserializeJson(const rapidjson::Value& obj)
{
    if (!obj.IsObject())
    {
        return false;
    }

    m_data.CopyFrom(obj, m_data.GetAllocator());

    if (obj.HasMember("MinDistance") && obj["MinDistance"].IsFloat())
        m_minDistance = obj["MinDistance"].GetFloat();
    if (obj.HasMember("DistanceDamage") && obj["DistanceDamage"].IsFloat())
        m_distanceDamage = obj["DistanceDamage"].GetFloat();
    if (obj.HasMember("DistanceInstaKill") && obj["DistanceInstaKill"].IsFloat())
        m_distanceInstaKill = obj["DistanceInstaKill"].GetFloat();
    if (obj.HasMember("BaseDamage") && obj["BaseDamage"].IsFloat())
        m_baseDamage = obj["BaseDamage"].GetFloat();
    if (obj.HasMember("MaxDamage") && obj["MaxDamage"].IsFloat())
        m_maxDamage = obj["MaxDamage"].GetFloat();
    if (obj.HasMember("RadiusThreshold") && obj["RadiusThreshold"].IsFloat())
        m_radiusThreshold = obj["RadiusThreshold"].GetFloat();

    return true;
}

rapidjson::Value BoundConfig::getJson(rapidjson::Document::AllocatorType& allocator) const
{
    rapidjson::Value obj(rapidjson::kObjectType);

    obj.AddMember("_typeName", rapidjson::Value("BoundConfig", allocator), allocator);
    obj.AddMember("MinDistance", m_minDistance, allocator);
    obj.AddMember("DistanceDamage", m_distanceDamage, allocator);
    obj.AddMember("DistanceInstaKill", m_distanceInstaKill, allocator);
    obj.AddMember("BaseDamage", m_baseDamage, allocator);
    obj.AddMember("MaxDamage", m_maxDamage, allocator);
    obj.AddMember("RadiusThreshold", m_radiusThreshold, allocator);

    return obj;
}

void BoundConfig::syncFromData()
{
    if (!m_data.IsObject())
        return;

    if (m_data.HasMember("MinDistance") && m_data["MinDistance"].IsFloat())
        m_minDistance = m_data["MinDistance"].GetFloat();
    if (m_data.HasMember("DistanceDamage") && m_data["DistanceDamage"].IsFloat())
        m_distanceDamage = m_data["DistanceDamage"].GetFloat();
    if (m_data.HasMember("DistanceInstaKill") && m_data["DistanceInstaKill"].IsFloat())
        m_distanceInstaKill = m_data["DistanceInstaKill"].GetFloat();
    if (m_data.HasMember("BaseDamage") && m_data["BaseDamage"].IsFloat())
        m_baseDamage = m_data["BaseDamage"].GetFloat();
    if (m_data.HasMember("MaxDamage") && m_data["MaxDamage"].IsFloat())
        m_maxDamage = m_data["MaxDamage"].GetFloat();
    if (m_data.HasMember("RadiusThreshold") && m_data["RadiusThreshold"].IsFloat())
        m_radiusThreshold = m_data["RadiusThreshold"].GetFloat();
}