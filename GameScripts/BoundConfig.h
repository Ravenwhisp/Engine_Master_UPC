#pragma once

#include "DataContainerAPI.h"

class BoundConfig : public DataContainer
{
    DECLARE_DATACONTAINER(BoundConfig)

public:
    BoundConfig() = default;
    explicit BoundConfig(AssetReference& id)
        : DataContainer(id)
    {
    }

    float m_minDistance = 70.0f;
    float m_distanceDamage = 80.0f;
    float m_distanceInstaKill = 100.0f;
    float m_baseDamage = 20.0f;
    float m_maxDamage = 40.0f;
    float m_radiusThreshold = 2.0f;
    float m_separationHapticHpGate = 0.5f;

    IMPLEMENT_DATACONTAINER_FIELDS(BoundConfig,
        SERIALIZED_FLOAT(m_minDistance, "Min Distance", 0.0f, 200.0f, 1.0f),
        SERIALIZED_FLOAT(m_distanceDamage, "Damage Distance", 0.0f, 200.0f, 1.0f),
        SERIALIZED_FLOAT(m_distanceInstaKill, "Insta Kill Distance", 0.0f, 200.0f, 1.0f),
        SERIALIZED_FLOAT(m_baseDamage, "Base Damage", 0.0f, 100.0f, 1.0f),
        SERIALIZED_FLOAT(m_maxDamage, "Max Damage", 0.0f, 100.0f, 1.0f),
        SERIALIZED_FLOAT(m_radiusThreshold, "Radius Threshold", 0.0f, 10.0f, 0.1f),
        SERIALIZED_FLOAT(m_separationHapticHpGate, "Separation Haptic HP Gate", 0.0f, 1.0f, 0.01f)
    )
};

