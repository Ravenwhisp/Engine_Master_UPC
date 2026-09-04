#include "pch.h"
#include "ShadowExecution.h"

#include "ReaperGauge.h"
#include "DeathCharacter.h"
#include "LyrielCharacter.h"
#include "CooperativeSound.h"
#include "PlayerState.h"
#include "PlayerAnimationController.h"
#include "EnemyDamageable.h"
#include "Bound.h"
#include "BoundConfig.h"
#include "ShadowExecutionConfig.h"

IMPLEMENT_SCRIPT_FIELDS(ShadowExecution,
    SERIALIZED_ASSET_REF(m_config, "Shadow Execution Config", AssetType::DATA_CONTAINER),
    SERIALIZED_ASSET_REF(m_particlePrefab, "Particle Prefab", AssetType::PREFAB),
    SERIALIZED_COMPONENT_REF(m_reaperGaugeBar, "Reaper Gauge UI", ComponentType::UISLIDER),
    SERIALIZED_COMPONENT_REF(m_executionCanvas, "Execution Canvas", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_executionSprite, "Execution Sprite", ComponentType::TRANSFORM2D),
    )

    ShadowExecution::ShadowExecution(GameObject* owner)
    : Script(owner)
{
}

void ShadowExecution::Start()
{
    m_reaperGaugeSlider = m_reaperGaugeBar.getReferencedComponent();
    m_executionTransform = m_executionCanvas.getReferencedComponent();
    m_executionTransform2D = m_executionSprite.getReferencedComponent();
    if (m_executionTransform)
    {
        GameObjectAPI::setActive(m_executionTransform->getOwner(), false);
    }

    m_reaperGauge = GameObjectAPI::findScript<ReaperGauge>(getOwner());
    if (m_reaperGauge == nullptr)
    {
        Debug::warn("[ShadowExecution] ReaperGauge not found on GameController. Add it as a sibling script.");
    }

    m_sound = GameObjectAPI::findScript<CooperativeSound>(getOwner());

    Bound* bound = GameObjectAPI::findScript<Bound>(getOwner());

    if (!bound)
    {
        Debug::warn("[ShadowExecution] Bound not found on GameController. Add it as a sibling script.");
    }
    else
    {
        m_maxRadius = bound->m_config.get()->m_minDistance * 0.5f;
    }

    m_shadowExecutionConfig = m_config.get();

    if (!m_shadowExecutionConfig)
    {
        Debug::error("[ShadowExecution] ShadowExecutionConfig is missing.");
        return;
    }

    cachePlayers();
}

void ShadowExecution::Update()
{
    const float dt = Time::getDeltaTime();

    // Actualizar y eliminar los prefabs de partículas cuando pase 1 segundo
    for (auto it = m_temporaryPrefabs.begin(); it != m_temporaryPrefabs.end(); )
    {
        it->lifetimeRemaining -= dt;
        if (it->lifetimeRemaining <= 0.0f)
        {
            if (it->gameObject != nullptr)
            {
                GameObjectAPI::removeGameObject(it->gameObject);
            }
            it = m_temporaryPrefabs.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (m_isActive)
    {
        updateExecution(dt);
        return;
    }

    if (Time::getTimeScale() == 0.0f)
    {
        return;
    }

    if (m_p0WindowTimer > 0.0f)
    {
        if (m_reaperGauge->isFull())
        {
            m_p0WindowTimer -= dt;
        }
        else
        {
            m_p0WindowTimer = 0.0f;
        }
    }
    if (m_p1WindowTimer > 0.0f)
    {
        if (m_reaperGauge->isFull())
        {
            m_p1WindowTimer -= dt;
        }
        else
        {
            m_p1WindowTimer = 0.0f;
        }
    }

    const bool p0IsOpen = m_p0WindowTimer > 0.0f;
    const bool p1IsOpen = m_p1WindowTimer > 0.0f;

    if (p0IsOpen && m_p0WindowTimer <= 0.0f)
        Debug::log("[ShadowExecution] Player 0 window expired.");
    if (p1IsOpen && m_p1WindowTimer <= 0.0f)
        Debug::log("[ShadowExecution] Player 1 window expired.");


    if (Input::isFaceButtonTopJustPressed(0))
    {
        if (m_reaperGauge->isFull())
        {
            m_p0WindowTimer = m_shadowExecutionConfig->m_timeWindow;
            Debug::log("[ShadowExecution] Player 0 pressed Triangle. Window open for %.1fs.", m_shadowExecutionConfig->m_timeWindow);
        }
        else
        {
            Debug::log("[ShadowExecution] Player 0 pressed Triangle but gauge not full (%.0f%%). Keep exploiting marks!",
                m_reaperGauge->getGaugePercent() * 100.0f);
        }
    }
    if (Input::isFaceButtonTopJustPressed(1))
    {
        if (m_reaperGauge->isFull())
        {
            m_p1WindowTimer = m_shadowExecutionConfig->m_timeWindow;
            Debug::log("[ShadowExecution] Player 1 pressed Triangle. Window open for %.1fs.", m_shadowExecutionConfig->m_timeWindow);
        }
        else
        {
            Debug::log("[ShadowExecution] Player 1 pressed Triangle but gauge not full (%.0f%%). Keep exploiting marks!",
                m_reaperGauge->getGaugePercent() * 100.0f);
        }
    }
    if (m_p0WindowTimer > 0.0f && m_p1WindowTimer > 0.0f && m_reaperGauge->isFull())
        tryTrigger();
}

void ShadowExecution::cachePlayers()
{
    if (m_deathCharacter == nullptr)
    {
        const std::vector<GameObject*> holders = SceneAPI::findAllGameObjectsWithScript<DeathCharacter>();
        if (!holders.empty())
            m_deathCharacter = GameObjectAPI::findScript<DeathCharacter>(holders[0]);
    }

    if (m_lyrielCharacter == nullptr)
    {
        const std::vector<GameObject*> holders = SceneAPI::findAllGameObjectsWithScript<LyrielCharacter>();
        if (!holders.empty())
            m_lyrielCharacter = GameObjectAPI::findScript<LyrielCharacter>(holders[0]);
    }
}

void ShadowExecution::tryTrigger()
{
    cachePlayers();

    if (m_deathCharacter == nullptr || m_lyrielCharacter == nullptr)
    {
        Debug::warn("[ShadowExecution] Cannot trigger: Death or Lyriel not found in scene.");
        return;
    }

    beginExecution();
}

void ShadowExecution::beginExecution()
{
    Transform* deathTransform = GameObjectAPI::getTransform(m_deathCharacter->getOwner());
    Transform* lyrielTransform = GameObjectAPI::getTransform(m_lyrielCharacter->getOwner());
    if (deathTransform == nullptr || lyrielTransform == nullptr)
        return;

    const Vector3 deathPos = TransformAPI::getGlobalPosition(deathTransform);
    const Vector3 lyrielPos = TransformAPI::getGlobalPosition(lyrielTransform);

    m_center = (deathPos + lyrielPos) * 0.5f;
    m_currentRadius = 0.0f;
    m_executionTimer = 0.0f;
    m_p0WindowTimer = 0.0f;
    m_p1WindowTimer = 0.0f;
    m_hitEnemies.clear();

    m_isActive = true;
    m_reaperGauge->consume();

    if (m_sound != nullptr)
    {
        m_sound->playShadowExecution();
    }

    GameObject* fxCenter = GameObjectAPI::instantiatePrefab(m_particlePrefab.m_id, m_center, Vector3::Zero);
    if (fxCenter)
    {
        m_temporaryPrefabs.push_back({ fxCenter, 1.0f });
    }

    lockPlayers(true);

    if (m_executionTransform)
    {
        GameObjectAPI::setActive(m_executionTransform->getOwner(), true);
    }

    Debug::log("[ShadowExecution] *** TRIGGERED *** Center (%.2f, %.2f, %.2f), max radius %.2f, duration %.1fs.",
        m_center.x, m_center.y, m_center.z, m_maxRadius, m_shadowExecutionConfig->m_executionDuration);
}

void ShadowExecution::updateExecution(float dt)
{
    m_executionTimer += dt;

    float progress = m_shadowExecutionConfig->m_executionDuration > 0.0f ? (m_executionTimer / m_shadowExecutionConfig->m_executionDuration) : 1.0f;
    if (progress > 1.0f) progress = 1.0f;

    m_currentRadius = progress * m_maxRadius;

    applyAoEDamage();

    if (m_executionTimer >= m_shadowExecutionConfig->m_executionDuration)
    {
        endExecution();
    }
    else
    {
        updateUI();
    }
}

void ShadowExecution::applyAoEDamage()
{
    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true);
    for (GameObject* enemy : enemies)
    {
        if (enemy == nullptr)
            continue;

        bool alreadyHit = false;
        for (GameObject* hit : m_hitEnemies)
        {
            if (hit == enemy)
            {
                alreadyHit = true;
                break;
            }
        }
        if (alreadyHit)
            continue;

        Transform* enemyTransform = GameObjectAPI::getTransform(enemy);
        if (enemyTransform == nullptr)
            continue;

        const Vector3 enemyPos = TransformAPI::getGlobalPosition(enemyTransform);
        const float   distance = Vector3::Distance(m_center, enemyPos);
        if (distance > m_currentRadius)
            continue;

        EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(enemy);
        if (damageable == nullptr || damageable->isDead())
        {
            m_hitEnemies.push_back(enemy);
            continue;
        }

        const ShadowExecutionPreview preview = calculatePreview(damageable);

        EnemyHitContext ctx;
        ctx.damage = preview.damage;
        ctx.attacker = nullptr;
        ctx.attackType = PlayerAttackType::ShadowExecution;

        damageable->playShadowExecutionHitPreview();
        damageable->takeDamage(ctx);

        m_hitEnemies.push_back(enemy);
    }
}

void ShadowExecution::endExecution()
{
    lockPlayers(false);

    m_isActive = false;
    m_executionTimer = 0.0f;
    m_currentRadius = 0.0f;
    m_hitEnemies.clear();

    Transform2DAPI::setAlpha(m_executionTransform2D, 0);
    Transform2DAPI::setScale(m_executionTransform2D, Vector2(0.0f, 0.0f));

    if (m_executionTransform)
    {
        GameObjectAPI::setActive(m_executionTransform->getOwner(), false);
    }

    Debug::log("[ShadowExecution] Execution finished.");
}

void ShadowExecution::lockPlayers(bool locked)
{
    CharacterBase* characters[] = { m_deathCharacter, m_lyrielCharacter };
    for (CharacterBase* character : characters)
    {
        if (character == nullptr)
            continue;

        character->setUsingAbility(locked);

        PlayerState* state = character->getPlayerState();
        if (state != nullptr)
        {
            if (locked)
            {
                if (!state->isDowned())
                    state->setState(PlayerStateType::AttackRecovery);
            }
            else if (state->isRecoveringAttack())
            {
                state->setState(PlayerStateType::Normal);
            }
        }

        if (locked)
        {
            PlayerAnimationController* anim = character->getAnimationController();
            if (anim != nullptr)
                anim->requestAttack();
        }
    }
}

void ShadowExecution::drawGizmo()
{
    if (!m_isActive)
        return;

    const Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

    const Vector3 expandingColor = Vector3(0.85f, 0.10f, 0.10f);
    const Vector3 maxColor = Vector3(0.40f, 0.00f, 0.40f);

    DebugDrawAPI::drawCircle(m_center, up, expandingColor, m_currentRadius, 48.0f);
    DebugDrawAPI::drawCircle(m_center, up, maxColor, m_maxRadius, 48.0f);
    DebugDrawAPI::drawPoint(m_center, Vector3(1.0f, 1.0f, 0.0f), 5.0f);
}

void ShadowExecution::updateUI()
{
    if (!m_executionTransform || !m_executionTransform2D || !m_reaperGaugeSlider)
    {
        return;
    }

    const float t = m_executionTimer / m_shadowExecutionConfig->m_executionDuration;
    SliderAPI::setFillAmount(m_reaperGaugeSlider, 1.0f - t);
    Transform2DAPI::setAlpha(m_executionTransform2D, t);
    Transform2DAPI::setScale(m_executionTransform2D, Vector2(m_currentRadius, m_currentRadius));
}

ShadowExecutionPreview ShadowExecution::calculatePreview(const EnemyDamageable* damageable) const
{
    ShadowExecutionPreview preview;

    if (!damageable || !m_shadowExecutionConfig)
    {
        return preview;
    }

    const float maxHp = damageable->getMaxHp();
    const float currentHp = damageable->getCurrentHp();

    if (maxHp <= 0.0f || currentHp <= 0.0f)
    {
        return preview;
    }

    const float thresholdMultiplier = damageable->getShadowExecutionThresholdMultiplier();
    const float effectiveThreshold = m_shadowExecutionConfig->m_instaKillThreshold * thresholdMultiplier;
    const float thresholdHp = maxHp * effectiveThreshold;
    const float percentageDamage = maxHp * m_shadowExecutionConfig->m_percentageDamage;
    const float selectedDamage = percentageDamage > m_shadowExecutionConfig->m_fixedDamage ? percentageDamage : m_shadowExecutionConfig->m_fixedDamage;

    const bool insideExecutionThreshold = currentHp <= thresholdHp;

    preview.damage = insideExecutionThreshold ? currentHp : selectedDamage;

    if (preview.damage > currentHp)
    {
        preview.damage = currentHp;
    }

    preview.willDie = preview.damage >= currentHp;
    const float resultingHp = currentHp - preview.damage;
    preview.resultingHpPercent = resultingHp / maxHp;

    return preview;
}

float ShadowExecution::getExecutionThresholdPercent(const EnemyDamageable* damageable) const
{
    if (!damageable || !m_shadowExecutionConfig)
    {
        return 0.0f;
    }

    const float thresholdMultiplier = damageable->getShadowExecutionThresholdMultiplier();
    const float effectiveThreshold = m_shadowExecutionConfig->m_instaKillThreshold * thresholdMultiplier;

    return std::clamp(effectiveThreshold, 0.0f, 1.0f);
}

IMPLEMENT_SCRIPT(ShadowExecution)