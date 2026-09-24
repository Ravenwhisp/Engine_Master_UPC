#include "pch.h"
#include "AelorinDamageable.h"

#include "AelorinBossController.h"
#include "AelorinAttackConfig.h"
#include "Transform2D.h"

#include <algorithm>
#include <cmath>

IMPLEMENT_SCRIPT_FIELDS_INHERITED(AelorinDamageable, EnemyDamageable,
    FIELD_GROUP_LABEL("Phase 2 Health Bar"),
    SERIALIZED_COMPONENT_REF(m_phase2HealthBarContainer, "Health Bar Container Phase 2", ComponentType::TRANSFORM2D),
    SERIALIZED_COMPONENT_REF(m_phase2HealthSlider, "Health Slider Phase 2", ComponentType::UISLIDER),
    SERIALIZED_COMPONENT_REF(m_phase2HealthSlider2, "Health Slider 2 Phase 2", ComponentType::UISLIDER)
)

AelorinDamageable::AelorinDamageable(GameObject* owner)
    : EnemyDamageable(owner)
{
}

void AelorinDamageable::Start()
{
    EnemyDamageable::Start();

    m_controller = GameObjectAPI::findScript<AelorinBossController>(getOwner());

    if (!m_controller)
    {
        Debug::warn("[AelorinDamageable] AelorinBossController not found.");
    }
    else
    {
        const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
        if (config)
        {
            m_maxHp = config->m_phase1MaxHp;
            m_currentHp = m_maxHp;

            // Refresh current HP bar after changing max HP
            setHealthBarContainerActive(getHealthBarContainerTransform(), true);
            setHealthBarContainerActive(m_phase2HealthBarContainer.getReferencedComponent(), false);
            setupUI();
        }
    }

    m_currentThresholdIndex = 0;
    m_thresholdLocked = false;
    m_phaseTransitionPending = false;
    m_allowFinalDeath = false;

    Debug::log("[AelorinDamageable] Phase 1 started. HP: %.0f / %.0f", m_currentHp, m_maxHp);
}

void AelorinDamageable::takeDamage(float amount)
{
    EnemyHitContext ctx;
    ctx.damage = amount;
    ctx.continuous = false;
    ctx.attacker = nullptr;
    ctx.attackType = PlayerAttackType::None;

    takeDamage(ctx);
}

void AelorinDamageable::takeDamage(const HitContext& ctx)
{
    if (m_isDead || m_invulnerable)
    {
        return;
    }

    if (m_phaseTransitionPending)
    {
        return;
    }

    const EnemyHitContext& enemyCtx = static_cast<const EnemyHitContext&>(ctx);

    resetLastShadowMarkResult();

    const bool shadowMarkExploited = processShadowMarkHit(enemyCtx.attackType);

    // Boss is already sitting on a threshold
    if (m_thresholdLocked)
    {
        if (shadowMarkExploited)
        {
            processThresholdBreak(enemyCtx);
        }

        return;
    }

    // Apply normal damage
    processNormalDamage(enemyCtx);

    // If same hit both exploited the Shadow Mark and reached the threshold -> break it
    if (shadowMarkExploited && m_thresholdLocked)
    {
        processThresholdBreak(enemyCtx);
    }
}

const std::vector<AelorinThreshold>& AelorinDamageable::getActiveThresholds() const
{
    if (m_controller && m_controller->isPhase2())
    {
        return m_phase2Thresholds;
    }

    return m_phase1Thresholds;
}

const AelorinThreshold* AelorinDamageable::getCurrentThreshold() const
{
    if (!hasCurrentThreshold())
    {
        return nullptr;
    }

    return &getActiveThresholds()[m_currentThresholdIndex];
}

bool AelorinDamageable::hasCurrentThreshold() const
{
    return m_currentThresholdIndex < getActiveThresholds().size();
}

float AelorinDamageable::getCurrentThresholdPercent() const
{
    const AelorinThreshold* threshold = getCurrentThreshold();

    if (!threshold)
    {
        return 0.0f;
    }

    return threshold->percent;
}

float AelorinDamageable::getCurrentThresholdHp() const
{
    return getMaxHp() * getCurrentThresholdPercent();
}

void AelorinDamageable::processNormalDamage(const EnemyHitContext& ctx)
{
    if (!hasCurrentThreshold())
    {
        Debug::warn("[AelorinDamageable] Normal damage ignored: no active threshold.");
        return;
    }

    if (m_thresholdLocked)
    {
        Debug::log("[AelorinDamageable] Damage ignored. Current threshold is locked.");
        return;
    }

    if (ctx.damage <= 0.0f)
    {
        return;
    }

    const float thresholdHp = getCurrentThresholdHp();

    // Normal attack must never reach 0 -> which would trigger onHPDepleted()
    // 0% threshold is represented by very small positive HP value

    const float effectiveThresholdHp = thresholdHp <= 0.0f ? 0.001f : thresholdHp;
    const float damageUntilThreshold = getCurrentHp() - effectiveThresholdHp;

    if (damageUntilThreshold <= 0.0f)
    {
        m_currentHp = effectiveThresholdHp;
        lockCurrentThreshold();
        return;
    }

    EnemyHitContext adjustedContext = ctx;
    (adjustedContext.damage) = (std::min)(ctx.damage, damageUntilThreshold);

    if (adjustedContext.damage <= 0.0f)
    {
        return;
    }

    applyDamageWithoutShadowMark(adjustedContext);

    if (getCurrentHp() <= effectiveThresholdHp)
    {
        m_currentHp = effectiveThresholdHp;
        lockCurrentThreshold();
    }
}

void AelorinDamageable::processThresholdBreak(const EnemyHitContext& ctx)
{
    const AelorinThreshold* threshold = getCurrentThreshold();

    if (!threshold)
    {
        return;
    }

    if (!m_thresholdLocked)
    {
        return;
    }

    Debug::log("[AelorinDamageable] Shadow Mark broke the %.0f%% threshold.", threshold->percent * 100.0f);

    switch (threshold->type)
    {
    case AelorinThresholdType::Standard:
    {
        advanceThreshold();
        
        if (m_controller)
        {
            m_controller->spawnHealthDrops();
            m_controller->requestThresholdStagger();
        }

        Debug::log("[AelorinDamageable] Standard threshold broken.");
        break;
    }
    case AelorinThresholdType::Fury:
    {
        advanceThreshold();

        if (m_controller)
        {
            m_controller->requestFury();
        }

        Debug::log("[AelorinDamageable] Fury threshold broken.");
        break;
    }
    case AelorinThresholdType::PhaseTransition:
    {
        if (m_controller)
        {
            m_controller->spawnHealthDrops();
        }

        requestPhaseTransition();
        break;
    }
    case AelorinThresholdType::FinalDeath:
    {
        handleFinalDeath(ctx);
        break;
    }
    default:
    {
        Debug::error("[AelorinDamageable] Unknown threshold type.");
        break;
    }
    }
}

void AelorinDamageable::lockCurrentThreshold()
{
    if (m_thresholdLocked)
    {
        return;
    }

    m_thresholdLocked = true;

    Debug::log("[AelorinDamageable] Threshold locked at %.0f%%.", getCurrentThresholdPercent() * 100.0f);
}

void AelorinDamageable::advanceThreshold()
{
    if (!hasCurrentThreshold())
    {
        return;
    }

    ++m_currentThresholdIndex;
    m_thresholdLocked = false;

    if (!hasCurrentThreshold())
    {
        Debug::error("[AelorinDamageable] Threshold index is out of range.");
        return;
    }

    Debug::log("[AelorinDamageable] Next threshold: %.0f%%", getCurrentThresholdPercent() * 100.0f);
}

void AelorinDamageable::requestPhaseTransition()
{
    if (m_phaseTransitionPending)
    {
        return;
    }

    m_phaseTransitionPending = true;
    m_thresholdLocked = true;

    Debug::log("[AelorinDamageable] Final Phase 1 threshold broken. Phase transition requested.");

    if (m_controller)
    {
        m_controller->requestPhaseTransition();
    }
}

void AelorinDamageable::beginPhase2()
{
    // Controller must set its phase to Phase2 before calling this function
    const AelorinAttackConfig* config = m_controller ? m_controller->getAelorinAttackConfig() : nullptr;
    if (config)
    {
        m_maxHp = config->m_phase2MaxHp;
        m_currentHp = m_maxHp;
    }

    m_currentThresholdIndex = 0;
    m_thresholdLocked = false;
    m_phaseTransitionPending = false;
    m_allowFinalDeath = false;
    m_isDead = false;

    setupPhase2HealthBar();

    Debug::log("[AelorinDamageable] Phase 2 initialized. HP: %.0f / %.0f | First threshold: %.0f%%", m_currentHp, m_maxHp, getCurrentThresholdPercent() * 100.0f);
}

bool AelorinDamageable::hasActiveThresholdAt(float percent) const
{
    constexpr float tolerance = 0.001f;

    // 0% threshold has special behaviour
    if (std::abs(percent) <= tolerance)
    {
        if (m_controller && m_controller->isPhase2())
        {
            // Final Death has been broken
            if (m_allowFinalDeath)
            {
                return false;
            }
        }
        else
        {
            // Phase transition broken
            if (m_phaseTransitionPending)
            {
                return false;
            }
        }
    }

    const std::vector<AelorinThreshold>& thresholds = getActiveThresholds();

    for (std::size_t i = m_currentThresholdIndex; i < thresholds.size(); ++i)
    {
        if (std::abs(thresholds[i].percent - percent) <= tolerance)
        {
            return true;
        }
    }

    return false;
}

void AelorinDamageable::handleFinalDeath(const EnemyHitContext& ctx)
{
    if (m_allowFinalDeath)
    {
        return;
    }

    m_allowFinalDeath = true;
    m_thresholdLocked = false;

    EnemyHitContext finalHit = ctx;
    
    // Damageable reaches 0 and starts the standard death flow
    finalHit.damage = getCurrentHp();

    Debug::log("[AelorinDamageable] Final Shadow Execution received. Death is allowed.");

    applyDamageWithoutShadowMark(finalHit);
}

void AelorinDamageable::setupPhase2HealthBar()
{
    Transform2D* phase1Container = getHealthBarContainerTransform();
    Transform2D* phase2Container = m_phase2HealthBarContainer.getReferencedComponent();

    setHealthBarContainerActive(phase1Container, false);
    setHealthBarContainerActive(phase2Container, true);

    bindHealthBarUI(phase2Container, m_phase2HealthSlider.getReferencedComponent(), m_phase2HealthSlider2.getReferencedComponent());
}

void AelorinDamageable::setHealthBarContainerActive(Transform2D* container, bool active)
{
    if (!container)
    {
        return;
    }

    GameObject* object = container->getOwner();
    if (!object)
    {
        return;
    }

    GameObjectAPI::setActive(object, active);
}

void AelorinDamageable::onHpDepleted()
{
    if (!m_allowFinalDeath)
    {
        // normal damage should never reach this
        m_currentHp = 0.0001f;
        m_isDead = false;

        lockCurrentThreshold();

        Debug::warn("[AelorinDamageable] HP depletion blocked");
        return;
    }

    EnemyDamageable::onHpDepleted();
}

IMPLEMENT_SCRIPT(AelorinDamageable)