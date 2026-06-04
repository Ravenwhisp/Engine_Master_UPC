#pragma once

#include "EnemyDeathState.h"

class LinkedDeathEnemyState : public EnemyDeathState
{
    DECLARE_SCRIPT(LinkedDeathEnemyState)

public:
    explicit LinkedDeathEnemyState(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

    FieldList getExposedFields() const override;

    void notifyLinkedDeath();

public:
    ComponentRef<Transform> m_linkedPartner;
    float m_graceWindow = 3.0f;
    float m_reviveHpPercent = 0.5f;

private:
    void findPartnerHandler();
    bool isPartnerDead() const;
    void reviveAndExit();

    LinkedDeathEnemyState* m_partnerHandler = nullptr;
    bool m_initialized = false;

    bool m_isWaitingForPartner = false;
    float m_pendingDeathTimer = 0.0f;
    bool m_partnerDiedNotification = false;
};
