#include "pch.h"
#include "ArcherArrowShooter.h"
#include "ArcherAttackConfig.h"
#include "RangedEnemyController.h"
#include "ArcherArrowProjectile.h"
#include "ArcherGuardParticles.h"

IMPLEMENT_SCRIPT_FIELDS(ArcherArrowShooter,
    SERIALIZED_ASSET_REF(m_arrowPrefab, "Arrow Prefab", AssetType::PREFAB)
)

ArcherArrowShooter::ArcherArrowShooter(GameObject* owner) : Script(owner) {}

void ArcherArrowShooter::Start()
{
    m_controller = GameObjectAPI::findScript<RangedEnemyController>(getOwner());
    m_animation  = AnimationAPI::getAnimationComponent(getOwner());
    m_particles  = GameObjectAPI::findScript<ArcherGuardParticles>(getOwner());

}

void ArcherArrowShooter::Update()
{
    if (!m_animation || !m_config || !m_arrowPrefab.m_id.isValid()) return;

    const char* state = AnimationAPI::getActiveStateName(m_animation);
    if (!state) return;

    const bool nowInAttack = (strcmp(state, "BASIC_ATTACK") == 0);

    if (nowInAttack && !m_inAttack)
    {
        m_timer    = 0.0f;
        m_fired    = false;
        m_inAttack = true;
    }

    if (!nowInAttack && m_inAttack)
    {
        if (m_arrowGO) { GameObjectAPI::removeGameObject(m_arrowGO); m_arrowGO = nullptr; }
        if (m_particles) m_particles->stopBasicAttackTrail();
        m_inAttack = false;
    }

    if (!m_inAttack) return;

    m_timer += Time::getDeltaTime();

    // ── Fire arrow at windup time ─────────────────────────────────────────────
    if (!m_fired && m_timer >= m_config.get()->m_basicAttackWindupTime)
    {
        Transform* archerT = GameObjectAPI::getTransform(getOwner());
        Transform* targetT = m_controller ? m_controller->getCurrentTarget() : nullptr;

        if (archerT && targetT)
        {
            Vector3 archerPos = TransformAPI::getGlobalPosition(archerT);
            Vector3 targetPos = TransformAPI::getGlobalPosition(targetT);

            Vector3 toTarget = targetPos - archerPos;
            toTarget.y = 0.0f;
            if (toTarget.LengthSquared() > 0.0001f) toTarget.Normalize();

            Vector3 spawnPos = archerPos;
            spawnPos.x      += toTarget.x * 0.5f;
            spawnPos.y      += 1.2f;
            spawnPos.z      += toTarget.z * 0.5f;

            Vector3 dest = targetPos;
            dest.y      += 1.0f;

            m_arrowGO = GameObjectAPI::instantiatePrefab(m_arrowPrefab.m_id, spawnPos, Vector3::Zero);
            if (m_arrowGO)
            {
                ArcherArrowProjectile* arrow = GameObjectAPI::findScript<ArcherArrowProjectile>(m_arrowGO);
                if (arrow) arrow->launch(spawnPos, dest, 10.0f);
            }

            if (m_particles) m_particles->spawnBasicAttackTrail(spawnPos);
        }
        m_fired = true;
    }

    // ── Sync trail to arrow each frame ────────────────────────────────────────
    if (m_arrowGO && m_particles)
    {
        Transform* arrowT = GameObjectAPI::getTransform(m_arrowGO);
        if (arrowT)
        {
            m_particles->syncBasicAttackTrail(
                TransformAPI::getGlobalPosition(arrowT),
                TransformAPI::getGlobalEulerDegrees(arrowT));
        }
    }

    // ── Remove arrow + trail once it arrives ──────────────────────────────────
    if (m_arrowGO)
    {
        ArcherArrowProjectile* arrow = GameObjectAPI::findScript<ArcherArrowProjectile>(m_arrowGO);
        if (arrow && arrow->hasArrived())
        {
            GameObjectAPI::removeGameObject(m_arrowGO);
            m_arrowGO = nullptr;
            if (m_particles) m_particles->stopBasicAttackTrail();
        }
    }
}

IMPLEMENT_SCRIPT(ArcherArrowShooter)
