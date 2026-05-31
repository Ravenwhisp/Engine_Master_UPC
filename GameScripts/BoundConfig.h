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
};