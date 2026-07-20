#include "pch.h"
#include "BarrierEnemyDamageable.h"
#include "Transform2D.h"
#include <algorithm>

static const char* barrierAttackTypeNames[] =
{
    "None",
    "DeathBasic",
    "DeathCharged",
    "DeathDash",
    "LyrielArrow",
    "LyrielVolley",
    "LyrielCharged",
    "ShadowExecution",
    "Environment"
};

constexpr int barrierAttackTypeCount = 9;

IMPLEMENT_SCRIPT_FIELDS_INHERITED(BarrierEnemyDamageable, EnemyDamageable,
    SERIALIZED_FLOAT_VECTOR(m_barriersThresholds, "Barrier Thresholds (%)"),
    SERIALIZED_ENUM_INT(m_requiredAttackType, "Barrier Break Attack", barrierAttackTypeNames, barrierAttackTypeCount),
    SERIALIZED_BOOL(m_shadowMarkExploitBreaksBarriers, "Shadow Mark Exploit Breaks Barriers"),
    SERIALIZED_ASSET_REF(m_barrierPrefab, "Barrier UI Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(m_minPos, "Barrier Min Pos (0% HP)", -1000.0f, 1000.0f, 1.0f),
    SERIALIZED_FLOAT(m_maxPos, "Barrier Max Pos (100% HP)", -1000.0f, 1000.0f, 1.0f),
    SERIALIZED_FLOAT(m_barrierUIHeight, "Barrier UI Height", -1000.0f, 1000.0f, 1.0f)
)

BarrierEnemyDamageable::BarrierEnemyDamageable(GameObject* owner)
    : EnemyDamageable(owner)
{
}

void BarrierEnemyDamageable::Start()
{
    EnemyDamageable::Start();
    buildBarriers();
    instantiateBarrierUIs();
}

void BarrierEnemyDamageable::buildBarriers()
{
    m_barriers.clear();
    m_nextBarrierIndex = 0;

    for (float threshold : m_barriersThresholds)
    {
        float pct = std::clamp(threshold, 0.0f, 1.0f);
        if (pct > 0.0f)
        {
            Barrier b;
            b.hpPercent = pct;
            b.broken = false;
            m_barriers.push_back(b);
        }
    }

    std::sort(m_barriers.begin(), m_barriers.end(),
        [](const Barrier& a, const Barrier& b) { return a.hpPercent > b.hpPercent; });
}

void BarrierEnemyDamageable::instantiateBarrierUIs()
{
    m_barrierUIs.clear();

    if (!m_barrierPrefab.m_id.isValid())
        return;

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    Transform* healthBarTransform = TransformAPI::findChildByName(ownerTransform, "Health Bar");
    if (!healthBarTransform)
    {
        Debug::warn("[Barrier] %s - Health Bar not found in hierarchy.", GameObjectAPI::getName(m_owner));
        return;
    }

    GameObject* healthBarObject = ComponentAPI::getOwner(healthBarTransform);

    for (const Barrier& barrier : m_barriers)
    {
        GameObject* uiObject = GameObjectAPI::instantiatePrefab(
            m_barrierPrefab.m_id,
            Vector3::Zero,
            Vector3::Zero,
            healthBarObject);

        if (!uiObject)
        {
            Debug::warn("[Barrier] %s - Failed to instantiate barrier UI.", GameObjectAPI::getName(m_owner));
            continue;
        }

        TransformAPI::setPosition(GameObjectAPI::getTransform(uiObject), Vector3::Zero);

        Transform2D* transform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(uiObject, ComponentType::TRANSFORM2D));
        if (transform2D)
        {
            float x = m_maxPos + (1.0f - barrier.hpPercent) * (m_minPos - m_maxPos);
            Transform2DAPI::setPosition(transform2D, { x, m_barrierUIHeight });
        }

        BarrierUI ui;
        ui.gameObject = uiObject;
        ui.hpPercent = barrier.hpPercent;
        m_barrierUIs.push_back(ui);
    }
}

void BarrierEnemyDamageable::destroyBrokenBarrierUI(size_t index)
{
    if (index >= m_barrierUIs.size())
        return;

    BarrierUI& ui = m_barrierUIs[index];
    if (ui.gameObject)
    {
        GameObjectAPI::removeGameObject(ui.gameObject);
        ui.gameObject = nullptr;
    }
}

float BarrierEnemyDamageable::getNextBarrierAbsoluteHp() const
{
    for (size_t i = 0; i < m_barriers.size(); ++i)
    {
        if (!m_barriers[i].broken)
        {
            return m_maxHp * m_barriers[i].hpPercent;
        }
    }
    return 0.0f;
}

void BarrierEnemyDamageable::takeDamage(float amount)
{
    EnemyHitContext hit;
    hit.damage = amount;
    hit.attackType = PlayerAttackType::Environment;
    takeDamage(hit);
}

bool BarrierEnemyDamageable::canBreakBarrier(PlayerAttackType attackType) const
{
    if (attackType == PlayerAttackType::ShadowMarkExploit)
    {
        return m_shadowMarkExploitBreaksBarriers;
    }

    return attackType == static_cast<PlayerAttackType>(m_requiredAttackType);
}

void BarrierEnemyDamageable::takeDamage(const HitContext& ctx)
{
    const EnemyHitContext& hit = static_cast<const EnemyHitContext&>(ctx);

    if (m_isDead || m_invulnerable || hit.damage <= 0.0f)
    {
        return;
    }

    float nextBarrierHp = getNextBarrierAbsoluteHp();

    if (nextBarrierHp <= 0.0f)
    {
        EnemyDamageable::takeDamage(hit);
        return;
    }

    const float hpBefore = m_currentHp;
    const float hpAfter = m_currentHp - hit.damage;

    if (hpAfter >= nextBarrierHp)
    {
        EnemyDamageable::takeDamage(hit);
        return;
    }

    if (!canBreakBarrier(hit.attackType))
    {
        const float allowedDamage = m_currentHp - nextBarrierHp;
        if (allowedDamage > 0.0f)
        {
            EnemyHitContext limitedHit = hit;
            limitedHit.damage = allowedDamage;
            EnemyDamageable::takeDamage(limitedHit);
        }

        Debug::log("[Barrier] %s blocked hit at %.0f%% HP. Required attack: %s",
            GameObjectAPI::getName(m_owner),
            (nextBarrierHp / m_maxHp) * 100.0f,
            barrierAttackTypeNames[static_cast<int>(m_requiredAttackType)]);
        return;
    }

    for (size_t i = 0; i < m_barriers.size(); ++i)
    {
        if (m_barriers[i].broken)
        {
            continue;
        }

        const float barrierHp = m_maxHp * m_barriers[i].hpPercent;
        if (hpAfter <= barrierHp && hpBefore >= barrierHp)
        {
            m_barriers[i].broken = true;
            m_nextBarrierIndex = i + 1;
            destroyBrokenBarrierUI(i);

            Debug::log("[Barrier] %s broke barrier at %.0f%% HP (%s, HP: %.1f -> %.1f)",
                GameObjectAPI::getName(m_owner),
                m_barriers[i].hpPercent * 100.0f,
                barrierAttackTypeNames[static_cast<int>(m_requiredAttackType)],
                hpBefore,
                hpAfter);
        }
    }

    EnemyDamageable::takeDamage(hit);
}

void BarrierEnemyDamageable::kill()
{
    if (hasActiveBarriers())
    {
        Debug::log("[Barrier] %s kill prevented by active barrier.",
            GameObjectAPI::getName(m_owner));
        return;
    }

    EnemyDamageable::kill();
}

IMPLEMENT_SCRIPT(BarrierEnemyDamageable)
