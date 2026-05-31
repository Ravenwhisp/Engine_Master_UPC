#include "pch.h"
#include "BoundConfig.h"

IMPLEMENT_DATACONTAINER(BoundConfig)
IMPLEMENT_DATACONTAINER_FIELDS(BoundConfig,
    SERIALIZED_FLOAT(m_minDistance, "Min Distance", 0.0f, 200.0f, 1.0f),
    SERIALIZED_FLOAT(m_distanceDamage, "Damage Distance", 0.0f, 200.0f, 1.0f),
    SERIALIZED_FLOAT(m_distanceInstaKill, "Insta Kill Distance", 0.0f, 200.0f, 1.0f),
    SERIALIZED_FLOAT(m_baseDamage, "Base Damage", 0.0f, 100.0f, 1.0f),
    SERIALIZED_FLOAT(m_maxDamage, "Max Damage", 0.0f, 100.0f, 1.0f),
    SERIALIZED_FLOAT(m_radiusThreshold, "Radius Threshold", 0.0f, 10.0f, 0.1f)
)