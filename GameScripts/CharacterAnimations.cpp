#include "pch.h"
#include "CharacterAnimations.h"

namespace
{
    std::string trimmed(const std::string& value)
    {
        const size_t start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        const size_t end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }
}

IMPLEMENT_SCRIPT(CharacterAnimations)

IMPLEMENT_SCRIPT_FIELDS(CharacterAnimations,
    FIELD_GROUP_COLLAPSE("Basic Attack",
        SERIALIZED_STRING_VECTOR(m_basicStates, "Basic Combo States"),
        SERIALIZED_STRING(m_basicRecoveryState, "Basic Recovery State"),
        SERIALIZED_FLOAT(m_basicSpeed, "Basic Speed", 0.0f, 5.0f, 0.01f),
        SERIALIZED_FLOAT(m_basicBlendIn, "Basic Blend In", 0.0f, 2.0f, 0.01f),
        SERIALIZED_FLOAT(m_basicActionPct, "Basic Action %", 0.0f, 1.0f, 0.01f),
        SERIALIZED_FLOAT(m_basicRecoverPct, "Basic Recover %", 0.0f, 1.0f, 0.01f)
    ),
    FIELD_GROUP_COLLAPSE("Charged Attack",
        SERIALIZED_STRING(m_chargedState, "Charged State"),
        SERIALIZED_FLOAT(m_chargedSpeed, "Charged Speed", 0.0f, 5.0f, 0.01f),
        SERIALIZED_FLOAT(m_chargedBlendIn, "Charged Blend In", 0.0f, 2.0f, 0.01f),
        SERIALIZED_FLOAT(m_chargedActionPct, "Charged Action %", 0.0f, 1.0f, 0.01f),
        SERIALIZED_FLOAT(m_chargedRecoverPct, "Charged Recover %", 0.0f, 1.0f, 0.01f),
        SERIALIZED_FLOAT(m_chargedHoldPct, "Charged Hold %", 0.0f, 1.0f, 0.01f)
    ),
    FIELD_GROUP_COLLAPSE("Special Ability",
        SERIALIZED_STRING(m_specialState, "Special State"),
        SERIALIZED_FLOAT(m_specialSpeed, "Special Speed", 0.0f, 5.0f, 0.01f),
        SERIALIZED_FLOAT(m_specialBlendIn, "Special Blend In", 0.0f, 2.0f, 0.01f),
        SERIALIZED_FLOAT(m_specialActionPct, "Special Action %", 0.0f, 1.0f, 0.01f),
        SERIALIZED_FLOAT(m_specialRecoverPct, "Special Recover %", 0.0f, 1.0f, 0.01f)
    ),
    FIELD_GROUP_COLLAPSE("Shadow Execution",
        SERIALIZED_STRING(m_shadowExecutionState, "Shadow State"),
        SERIALIZED_FLOAT(m_shadowExecutionSpeed, "Shadow Speed", 0.0f, 5.0f, 0.01f),
        SERIALIZED_FLOAT(m_shadowExecutionBlendIn, "Shadow Blend In", 0.0f, 2.0f, 0.01f),
        SERIALIZED_FLOAT(m_shadowExecutionActionPct, "Shadow Action %", 0.0f, 1.0f, 0.01f),
        SERIALIZED_FLOAT(m_shadowExecutionRecoverPct, "Shadow Recover %", 0.0f, 1.0f, 0.01f)
    )
)

AttackAnimInfo CharacterAnimations::resolve(AttackAnimId id, int variant) const
{
    AttackAnimInfo info;

    switch (id)
    {
    case AttackAnimId::Basic:
        if (!m_basicStates.empty())
        {
            // Cycle through the combo clips (1-2-1-2...) instead of clamping on the last.
            const int size = static_cast<int>(m_basicStates.size());
            const int index = ((variant % size) + size) % size;
            info.stateName = m_basicStates[index];
        }
        info.speed = m_basicSpeed;
        info.blendIn = m_basicBlendIn;
        info.actionPct = m_basicActionPct;
        info.recoverPct = m_basicRecoverPct;
        info.recoveryState = m_basicRecoveryState;
        break;

    case AttackAnimId::Charged:
        info.stateName = m_chargedState;
        info.speed = m_chargedSpeed;
        info.blendIn = m_chargedBlendIn;
        info.actionPct = m_chargedActionPct;
        info.recoverPct = m_chargedRecoverPct;
        info.holdPct = m_chargedHoldPct;
        break;

    case AttackAnimId::Special:
        info.stateName = m_specialState;
        info.speed = m_specialSpeed;
        info.blendIn = m_specialBlendIn;
        info.actionPct = m_specialActionPct;
        info.recoverPct = m_specialRecoverPct;
        break;

    case AttackAnimId::ShadowExecution:
        info.stateName = m_shadowExecutionState;
        info.speed = m_shadowExecutionSpeed;
        info.blendIn = m_shadowExecutionBlendIn;
        info.actionPct = m_shadowExecutionActionPct;
        info.recoverPct = m_shadowExecutionRecoverPct;
        break;
    }

    info.stateName = trimmed(info.stateName);
    info.recoveryState = trimmed(info.recoveryState);
    return info;
}
