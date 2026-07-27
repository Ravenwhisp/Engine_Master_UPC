#pragma once

#include "DataContainerAPI.h"

class EnemyBaseDataConfig : public DataContainer
{
    DECLARE_DATACONTAINER(EnemyBaseDataConfig)

public:
    EnemyBaseDataConfig() = default;
    explicit EnemyBaseDataConfig(AssetId& id)
        : DataContainer(id)
    {
    }

    float m_maxHp = 100.0f;
    float m_recoveryDuration = 0.75f;
    float m_stunnedDuration = 2.0f;
    float m_moveSpeed = 3.5f;

    IMPLEMENT_DATACONTAINER_FIELDS(EnemyBaseDataConfig,
        FIELD_GROUP_COLLAPSE("Health",
            SERIALIZED_FLOAT(m_maxHp, "Max HP", 1.0f, 9999.0f, 1.0f)
        ),
        FIELD_GROUP_COLLAPSE("Combat",
            SERIALIZED_FLOAT(m_recoveryDuration, "Recovery Duration", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_stunnedDuration, "Stunned Duration", 0.0f, 10.0f, 0.05f)
        ),
        FIELD_GROUP_COLLAPSE("Movement",
            SERIALIZED_FLOAT(m_moveSpeed, "Move Speed", 0.0f, 50.0f, 0.1f)
        )
    )
};
