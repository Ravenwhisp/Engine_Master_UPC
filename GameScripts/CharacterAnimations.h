#pragma once

#include "ScriptAPI.h"

#include <string>
#include <vector>

enum class AttackAnimId
{
    Basic = 0,
    Charged,
    Special,
    ShadowExecution
};

struct AttackAnimInfo
{
    std::string stateName;
    float speed = 1.0f;
    float blendIn = 0.15f;
    float actionPct = 0.30f;
    float recoverPct = 0.90f;
    std::string recoveryState;
    float holdPct = 0.5f;
};

class CharacterAnimations : public Script
{
    DECLARE_SCRIPT(CharacterAnimations)

public:
    explicit CharacterAnimations(GameObject* owner) : Script(owner) {}

    FieldList getExposedFields() const override;

    AttackAnimInfo resolve(AttackAnimId id, int variant) const;

    std::vector<std::string> m_basicStates;
    std::string m_basicRecoveryState = "";
    float m_basicSpeed = 1.0f;
    float m_basicBlendIn = 0.15f;
    float m_basicActionPct = 0.35f;
    float m_basicRecoverPct = 0.90f;

    std::string m_chargedState = "";
    float m_chargedSpeed = 1.0f;
    float m_chargedBlendIn = 0.15f;
    float m_chargedActionPct = 0.50f;
    float m_chargedRecoverPct = 0.90f;
    float m_chargedHoldPct = 0.5f;

    std::string m_specialState = "";
    float m_specialSpeed = 1.0f;
    float m_specialBlendIn = 0.15f;
    float m_specialActionPct = 0.35f;
    float m_specialRecoverPct = 0.90f;

    std::string m_shadowExecutionState = "";
    float m_shadowExecutionSpeed = 1.0f;
    float m_shadowExecutionBlendIn = 0.15f;
    float m_shadowExecutionActionPct = 0.35f;
    float m_shadowExecutionRecoverPct = 0.90f;
};
