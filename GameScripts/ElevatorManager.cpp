#include "pch.h"
#include "ElevatorManager.h"
#include "CombatAreaEvent.h"
#include "CrystalShadowMark.h"
#include "Damageable.h"

IMPLEMENT_SCRIPT_FIELDS(ElevatorManager,
    SERIALIZED_COMPONENT_REF_VECTOR(m_crystals, "Crystals", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF_VECTOR(m_combatAreaRoots, "Combat Area Roots", ComponentType::TRANSFORM),
    FIELD_GROUP_COLLAPSE("Walls",
        SERIALIZED_COMPONENT_REF(m_wallTop, "Wall Top", ComponentType::TRANSFORM),
        SERIALIZED_COMPONENT_REF(m_wallBottom, "Wall Bottom", ComponentType::TRANSFORM),
        SERIALIZED_FLOAT(m_wallPieceHeight, "Wall Piece Height", 0.1f, 1000.0f, 0.1f),
        SERIALIZED_FLOAT(m_wallThreshold, "Wall Threshold", 0.0f, 10000.0f, 0.1f),
        SERIALIZED_FLOAT(m_wallScrollSpeed, "Wall Scroll Speed", 0.0f, 100.0f, 0.1f)
    ),
    FIELD_GROUP_COLLAPSE("Platform",
        SERIALIZED_COMPONENT_REF(m_platform, "Platform", ComponentType::TRANSFORM),
        SERIALIZED_COMPONENT_REF_VECTOR(m_platformTargets, "Platform Targets", ComponentType::TRANSFORM),
        SERIALIZED_FLOAT(m_platformMoveDuration, "Move Duration", 0.0f, 30.0f, 0.05f),
        SERIALIZED_FLOAT(m_platformLerpPower, "Lerp Power", 0.1f, 10.0f, 0.1f)
    ),
    SERIALIZED_INT(m_wavesPerCycle, "Waves Per Cycle")
)

ElevatorManager::ElevatorManager(GameObject* owner)
    : Script(owner)
{
}

void ElevatorManager::Start()
{
    resolveCombatAreas();
    resolveCrystals();

    const int areaCount = static_cast<int>(m_combatAreas.size());
    for (int i = 0; i < areaCount; i++)
        disableArea(i);
}

void ElevatorManager::Update()
{
    const bool cheatHeld = Input::isKeyDown(KeyCode::RightShift) && Input::isKeyDown(KeyCode::L);

    if (cheatHeld && !m_cheatWasPressed)
        killWaveEnemies(m_cheatWaveIndex++);

    m_cheatWasPressed = cheatHeld;

    const int areaCount = static_cast<int>(m_combatAreas.size());
    const int targetCount = static_cast<int>(m_platformTargets.size());

    switch (m_state)
    {
    case State::Idle:
    {
        const int crystalA = m_currentCycle * 2;
        const int crystalB = crystalA + 1;
        const int crystalCount = static_cast<int>(m_crystalScripts.size());

        if (crystalA >= crystalCount || crystalB >= crystalCount)
            break;

        CrystalShadowMark* a = m_crystalScripts[crystalA];
        CrystalShadowMark* b = m_crystalScripts[crystalB];

        if (m_waitingForReset)
        {
            if (a != nullptr && b != nullptr && !a->isActivated() && !b->isActivated())
                m_waitingForReset = false;
            break;
        }

        if (a != nullptr && b != nullptr && a->isActivated() && b->isActivated())
        {
            if (m_currentCycle * 2 < targetCount)
                startPlatformMove(m_currentCycle * 2);

            m_movingToCombat = true;
            m_wallsActive = true;
            m_state = State::PlatformMoving;
        }
        break;
    }

    case State::PlatformMoving:
    {
        updateWallScroll();
        updatePlatformMove();

        if (!m_platformMoving)
        {
            if (m_movingToCombat)
            {
                beginWave(m_wavesCompleted);
                m_wavesDoneInCycle = 1;
                m_state = State::CycleActive;
            }
            else
            {
                m_currentCycle++;

                if (m_currentCycle * 2 >= targetCount || m_wavesCompleted >= areaCount)
                    m_state = State::Done;
                else
                {
                    m_waitingForReset = true;
                    m_state = State::Idle;
                }
            }
        }
        break;
    }

    case State::CycleActive:
    {
        updateWallScroll();

        if (m_wavesCompleted < areaCount)
        {
            CombatAreaEvent* area = m_combatAreas[m_wavesCompleted];
            if (area != nullptr && area->hasCompleted())
            {
                m_wavesCompleted++;
                m_wavesDoneInCycle++;

                if (m_wavesDoneInCycle <= m_wavesPerCycle && m_wavesCompleted < areaCount)
                {
                    beginWave(m_wavesCompleted);
                }

                if (m_wavesDoneInCycle > m_wavesPerCycle)
                {
                    m_wallsActive = false;

                    if (m_currentCycle * 2 + 1 < targetCount)
                        startPlatformMove(m_currentCycle * 2 + 1);

                    m_movingToCombat = false;
                    m_state = State::PlatformMoving;
                }
            }
        }
        break;
    }

    case State::Done:
        break;
    }
}

void ElevatorManager::resolveCombatAreas()
{
    m_combatAreas.clear();

    for (auto& rootRef : m_combatAreaRoots)
    {
        Transform* rootTransform = rootRef.getReferencedComponent();
        if (rootTransform == nullptr)
        {
            m_combatAreas.push_back(nullptr);
            continue;
        }

        GameObject* rootObject = ComponentAPI::getOwner(rootTransform);
        if (rootObject == nullptr)
        {
            m_combatAreas.push_back(nullptr);
            continue;
        }

        CombatAreaEvent* area = GameObjectAPI::findScript<CombatAreaEvent>(rootObject);
        m_combatAreas.push_back(area);
    }
}

void ElevatorManager::resolveCrystals()
{
    m_crystalScripts.clear();

    for (auto& ref : m_crystals)
    {
        Transform* t = ref.getReferencedComponent();
        if (t == nullptr)
        {
            m_crystalScripts.push_back(nullptr);
            continue;
        }

        GameObject* obj = ComponentAPI::getOwner(t);
        if (obj == nullptr)
        {
            m_crystalScripts.push_back(nullptr);
            continue;
        }

        CrystalShadowMark* crystal = GameObjectAPI::findScript<CrystalShadowMark>(obj);
        m_crystalScripts.push_back(crystal);
    }
}

void ElevatorManager::enableArea(int waveIndex)
{
    if (waveIndex < 0 || waveIndex >= static_cast<int>(m_combatAreas.size()))
        return;

    CombatAreaEvent* area = m_combatAreas[waveIndex];
    if (area == nullptr)
        return;

    for (auto& enemyRef : area->m_enemies)
    {
        Transform* t = enemyRef.getReferencedComponent();
        if (t == nullptr) continue;
        GameObject* obj = ComponentAPI::getOwner(t);
        if (obj == nullptr) continue;
        setActiveRecursive(obj, true);
    }
}

void ElevatorManager::disableArea(int waveIndex)
{
    if (waveIndex < 0 || waveIndex >= static_cast<int>(m_combatAreas.size()))
        return;

    CombatAreaEvent* area = m_combatAreas[waveIndex];
    if (area == nullptr)
        return;

    for (auto& enemyRef : area->m_enemies)
    {
        Transform* t = enemyRef.getReferencedComponent();
        if (t == nullptr) continue;
        GameObject* obj = ComponentAPI::getOwner(t);
        if (obj == nullptr) continue;
        setActiveRecursive(obj, false);
    }
}

void ElevatorManager::setActiveRecursive(GameObject* obj, bool active)
{
    if (obj == nullptr)
        return;

    GameObjectAPI::setActive(obj, active);

    Transform* t = GameObjectAPI::getTransform(obj);
    if (t == nullptr)
        return;

    const int childCount = TransformAPI::getChildCount(t);
    for (int i = 0; i < childCount; i++)
    {
        Transform* child = TransformAPI::getChild(t, i);
        if (child == nullptr)
            continue;
        GameObject* childObj = ComponentAPI::getOwner(child);
        setActiveRecursive(childObj, active);
    }
}

void ElevatorManager::beginWave(int waveIndex)
{
    if (waveIndex < 0 || waveIndex >= static_cast<int>(m_combatAreas.size()))
        return;

    CombatAreaEvent* area = m_combatAreas[waveIndex];
    if (area == nullptr)
        return;

    enableArea(waveIndex);
    area->executeEvent(nullptr);
}

void ElevatorManager::startWallScroll()
{
    m_wallsActive = true;
}

void ElevatorManager::updateWallScroll()
{
    if (!m_wallsActive)
        return;

    Transform* top = m_wallTop.getReferencedComponent();
    Transform* bottom = m_wallBottom.getReferencedComponent();

    if (top == nullptr || bottom == nullptr)
        return;

    const float dt = Time::getDeltaTime();
    const float scrollAmount = m_wallScrollSpeed * dt;

    Vector3 topPos = TransformAPI::getPosition(top);
    Vector3 bottomPos = TransformAPI::getPosition(bottom);

    topPos.y += scrollAmount;
    bottomPos.y += scrollAmount;

    if (topPos.y >= m_wallThreshold)
        topPos.y -= m_wallPieceHeight * 2.0f;

    if (bottomPos.y >= m_wallThreshold)
        bottomPos.y -= m_wallPieceHeight * 2.0f;

    TransformAPI::setPosition(top, topPos);
    TransformAPI::setPosition(bottom, bottomPos);
}

void ElevatorManager::startPlatformMove(int targetIndex)
{
    if (targetIndex < 0 || targetIndex >= static_cast<int>(m_platformTargets.size()))
        return;

    Transform* platformTransform = m_platform.getReferencedComponent();
    if (platformTransform == nullptr)
        return;

    Transform* targetTransform = m_platformTargets[targetIndex].getReferencedComponent();
    if (targetTransform == nullptr)
        return;

    m_platformStartY = TransformAPI::getPosition(platformTransform).y;
    m_platformTargetY = TransformAPI::getPosition(targetTransform).y;

    m_platformTimer = 0.0f;
    m_platformMoving = true;
}

void ElevatorManager::updatePlatformMove()
{
    if (!m_platformMoving)
        return;

    Transform* platformTransform = m_platform.getReferencedComponent();
    if (platformTransform == nullptr)
    {
        m_platformMoving = false;
        return;
    }

    float dt = Time::getDeltaTime();
    m_platformTimer += dt;
    float alpha = m_platformTimer / m_platformMoveDuration;
    if (alpha > 1.0f)
        alpha = 1.0f;

    alpha = pow(alpha, m_platformLerpPower);

    Vector3 currentPos = TransformAPI::getPosition(platformTransform);
    currentPos.y = m_platformStartY + (m_platformTargetY - m_platformStartY) * alpha;
    TransformAPI::setPosition(platformTransform, currentPos);

    if (m_platformTimer >= m_platformMoveDuration)
        m_platformMoving = false;
}

int ElevatorManager::getTotalWaves() const
{
    return static_cast<int>(m_platformTargets.size()) * m_wavesPerCycle;
}

void ElevatorManager::killWaveEnemies(int waveIndex)
{
    if (waveIndex < 0 || waveIndex >= static_cast<int>(m_combatAreas.size()))
        return;

    CombatAreaEvent* area = m_combatAreas[waveIndex];
    if (area == nullptr)
        return;

    for (auto& enemyRef : area->m_enemies)
    {
        Transform* t = enemyRef.getReferencedComponent();
        if (t == nullptr) continue;
        GameObject* obj = ComponentAPI::getOwner(t);
        if (obj == nullptr) continue;
        Damageable* damageable = GameObjectAPI::findScript<Damageable>(obj);
        if (damageable == nullptr) continue;
        if (damageable->isDead()) continue;
        damageable->takeDamage(damageable->getCurrentHp());
    }

    Debug::log("[ElevatorManager] Cheat: killed wave %d enemies", waveIndex);
}

IMPLEMENT_SCRIPT(ElevatorManager)
