#pragma once

#include "DataContainerAPI.h"

class ShadowExecutionConfig : public DataContainer
{
    DECLARE_DATACONTAINER(ShadowExecutionConfig)

public:
    ShadowExecutionConfig() = default;
    explicit ShadowExecutionConfig(AssetId& id) : DataContainer(id) {}

    float m_timeWindow = 2.5f;
    float m_executionDuration = 1.0f;
    float m_instaKillThreshold = 0.40f;
    float m_fixedDamage = 35.0f;
    float m_percentageDamage = 0.10f;

    IMPLEMENT_DATACONTAINER_FIELDS(ShadowExecutionConfig,
        FIELD_GROUP_COLLAPSE("Timing",
            SERIALIZED_FLOAT(m_timeWindow, "Co-op Window (s)", 0.1f, 10.0f, 0.1f),
            SERIALIZED_FLOAT(m_executionDuration, "Execution Duration (s)", 0.1f, 10.0f, 0.1f)
        ),
        FIELD_GROUP_COLLAPSE("Damage",
            SERIALIZED_FLOAT(m_instaKillThreshold, "Insta Kill HP %", 0.0f, 1.0f, 0.01f),
            SERIALIZED_FLOAT(m_fixedDamage, "Fixed Damage", 0.0f, 1000.0f, 1.0f),
            SERIALIZED_FLOAT(m_percentageDamage, "Percentage Damage (max HP %)", 0.0f, 1.0f, 0.01f)
        )
    )
};