#include "pch.h"
#include "DeathTauntChains.h"
#include "DeathParticles.h"
#include "EnemyDamageable.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kPi = 3.14159265f;
    constexpr float kRadToDeg = 180.0f / kPi;
    constexpr float kOneShotLifetime = 1.5f;

    // Hidden links stay active
    // so they are shrunk to almost nothing and parked under the floor instead.
    // Not exactly zero: a zero scale breaks matrix decomposition and normal matrices.
    constexpr float kHiddenScale = 0.0001f;
    const Vector3 kHiddenOffset(0.0f, -2.0f, 0.0f);

    float clamp01(float value)
    {
        return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    }
}

DeathTauntChains::DeathTauntChains(DeathParticles& owner)
    : m_owner(owner)
{
}

void DeathTauntChains::ensurePool()
{
    if (m_poolBuilt)
    {
        return;
    }

    m_poolBuilt = true;

    const AssetId& linkPrefab = m_owner.m_tauntChainLinkPrefab.m_id;
    if (!linkPrefab.isValid())
    {
        Debug::warn("[DeathTauntChains] Taunt Chain Link Prefab is not assigned on DeathParticles. Taunt will play without chains.");
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(m_owner.getOwner());
    m_handBone = ParticleLifecycle::findChildRecursive(ownerTransform, m_owner.m_tauntChainHandBone.c_str());

    const int chainCount = m_owner.m_tauntChainMaxChains > 0 ? m_owner.m_tauntChainMaxChains : 1;
    const float spacing = m_owner.m_tauntChainLinkSpacing > 0.01f ? m_owner.m_tauntChainLinkSpacing : 0.01f;
    const int linksPerChain = static_cast<int>(m_owner.m_tauntChainMaxLength / spacing) + 2;

    m_root = GameObjectAPI::createGameObject("TauntChains", ParticleLifecycle::getRuntimeVfxContainer());
    const Vector3 park = getStart() + kHiddenOffset;

    for (int c = 0; c < chainCount; ++c)
    {
        Chain chain;
        chain.phase = static_cast<float>(c) * 2.1f;

        for (int i = 0; i < linksPerChain; ++i)
        {
            GameObject* link = GameObjectAPI::instantiatePrefab(linkPrefab, park, Vector3::Zero, m_root);
            if (!link)
            {
                Debug::warn("[DeathTauntChains] Could not spawn the chain link prefab. Taunt will play without chains.");
                destroyPool();
                m_poolBuilt = true; // don't retry every frame
                return;
            }

            TransformAPI::setScale(GameObjectAPI::getTransform(link), Vector3(kHiddenScale, kHiddenScale, kHiddenScale));
            chain.links.push_back(link);
            chain.linkShown.push_back(false);
        }

        m_chains.push_back(chain);
    }
}

void DeathTauntChains::destroyPool()
{
    m_chains.clear();

    if (m_root && SceneAPI::containsGameObject(m_root))
    {
        GameObjectAPI::removeGameObject(m_root);
    }

    m_root = nullptr;
    m_rootSeenInScene = false;
    m_handBone = nullptr;
    m_poolBuilt = false;
}

void DeathTauntChains::shutdown()
{
    m_oneShots.clear();
    destroyPool();
}

void DeathTauntChains::launch(const std::vector<GameObject*>& targets, float travelTime)
{
    ensurePool();

    if (m_chains.empty() || targets.empty())
    {
        return;
    }

    // Nearest enemies get the chains first.
    const Vector3 origin = getStart();
    std::vector<GameObject*> sorted = targets;
    std::sort(sorted.begin(), sorted.end(), [&](GameObject* a, GameObject* b)
    {
        const Vector3 da = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(a)) - origin;
        const Vector3 db = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(b)) - origin;
        return da.LengthSquared() < db.LengthSquared();
    });

    size_t chainIndex = 0;
    for (GameObject* target : sorted)
    {
        if (!isTargetValid(target))
        {
            continue;
        }

        while (chainIndex < m_chains.size() && m_chains[chainIndex].state != State::Idle)
        {
            ++chainIndex;
        }

        if (chainIndex >= m_chains.size())
        {
            break;
        }

        Chain& chain = m_chains[chainIndex];
        chain.target = target;
        chain.state = State::Shooting;
        chain.pull = PullState::None;
        chain.timer = 0.0f;
        chain.travelTime = travelTime > 0.01f ? travelTime : 0.01f;
        chain.grabBurstPlayed = false;
        chain.tip = origin;
    }
}

void DeathTauntChains::cancel()
{
    for (Chain& chain : m_chains)
    {
        if (chain.state == State::Shooting || chain.state == State::Latched)
        {
            startRetract(chain);
        }
    }
}

void DeathTauntChains::onPullStarted(GameObject* enemy)
{
    if (Chain* chain = findChain(enemy))
    {
        chain->pull = PullState::Pulling;
    }
}

void DeathTauntChains::onPullFinished(GameObject* enemy)
{
    if (Chain* chain = findChain(enemy))
    {
        chain->pull = PullState::Finished;
    }
}

DeathTauntChains::Chain* DeathTauntChains::findChain(GameObject* enemy)
{
    for (Chain& chain : m_chains)
    {
        if (chain.target == enemy && (chain.state == State::Shooting || chain.state == State::Latched))
        {
            return &chain;
        }
    }

    return nullptr;
}

void DeathTauntChains::update(float deltaTime)
{
    m_oneShots.update(deltaTime);

    // Build the pool up front so the first taunt doesn't hitch.
    if (!m_poolBuilt)
    {
        ensurePool();
    }

    // The pool lives under the runtime VFX container; if that got destroyed, start again.
    // Objects created during the scene update are only added at the end of the frame,
    // so the root only counts as gone once it has been seen in the scene at least once.
    if (m_root)
    {
        if (SceneAPI::containsGameObject(m_root))
        {
            m_rootSeenInScene = true;
        }
        else if (m_rootSeenInScene)
        {
            m_chains.clear();
            m_root = nullptr;
            m_rootSeenInScene = false;
            m_poolBuilt = false;
            return;
        }
    }

    if (m_chains.empty())
    {
        return;
    }

    m_clock += deltaTime;
    const Vector3 start = getStart();

    for (Chain& chain : m_chains)
    {
        if (chain.state != State::Idle)
        {
            updateChain(chain, deltaTime, start);
        }
    }
}

void DeathTauntChains::updateChain(Chain& chain, float deltaTime, const Vector3& start)
{
    chain.timer += deltaTime;
    float whip = 0.0f;

    switch (chain.state)
    {
    case State::Shooting:
    {
        if (!isTargetValid(chain.target))
        {
            startRetract(chain);
            break;
        }

        // Fast out; the whip settles as it arrives.
        const float t = clamp01(chain.timer / chain.travelTime);
        const float eased = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, t);
        const Vector3 contact = getContactPoint(chain.target);
        chain.tip = start + (contact - start) * eased;
        whip = m_owner.m_tauntChainWhip * (1.0f - eased);

        if (t >= 1.0f)
        {
            chain.state = State::Latched;
            chain.timer = 0.0f;

            // Impact at the front of the enemy; Death's grab follows a moment later.
            m_owner.playHitFlash(getEnemyCenter(chain.target), chain.target);
            ParticleLifecycle::spawnOneShotTimed(m_oneShots, m_owner.m_tauntChainContactPuffPrefab.m_id, contact, Vector3::Zero, kOneShotLifetime);
        }
        break;
    }

    case State::Latched:
    {
        if (!isTargetValid(chain.target))
        {
            startRetract(chain);
            break;
        }

        if (!chain.grabBurstPlayed && chain.timer >= m_owner.m_tauntChainGrabBurstDelay)
        {
            chain.grabBurstPlayed = true;
            // Parented to the enemy so it rides along while they're dragged in.
            ParticleLifecycle::spawnOneShotTimed(m_oneShots, m_owner.m_tauntChainGrabBurstPrefab.m_id, getEnemyCenter(chain.target), Vector3::Zero, kOneShotLifetime, chain.target);
        }

        // Small sideways shake at the enemy end reads as tension.
        Vector3 tip = getContactPoint(chain.target);
        Vector3 side(start.z - tip.z, 0.0f, tip.x - start.x);
        if (side.LengthSquared() > 0.0001f)
        {
            side.Normalize();
            tip = tip + side * (std::sin(m_clock * 55.0f + chain.phase) * m_owner.m_tauntChainTension);
        }
        chain.tip = tip;

        const bool pullFinished = chain.pull == PullState::Finished;
        const bool neverPulled = chain.pull == PullState::None && chain.timer >= m_owner.m_tauntChainMinLatchTime;

        if (pullFinished || neverPulled || chain.timer >= m_owner.m_tauntChainMaxLatchTime)
        {
            startRetract(chain);
        }
        break;
    }

    case State::Retracting:
    {
        // Accelerates back into Death, like it's being reeled in, with a little slack.
        const float t = clamp01(chain.timer / m_owner.m_tauntChainRetractTime);
        const float eased = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInCubic, t);
        chain.tip = chain.retractFrom + (start - chain.retractFrom) * eased;
        whip = m_owner.m_tauntChainWhip * 0.3f * (1.0f - t);

        if (t >= 1.0f)
        {
            chain.state = State::Idle;
            chain.target = nullptr;
            hide(chain, start + kHiddenOffset);
            return;
        }
        break;
    }

    default:
        return;
    }

    if (chain.state != State::Idle)
    {
        layout(chain, start, whip);
    }
}

void DeathTauntChains::startRetract(Chain& chain)
{
    chain.state = State::Retracting;
    chain.timer = 0.0f;
    chain.retractFrom = chain.tip;
    chain.target = nullptr;
    chain.pull = PullState::None;
}

void DeathTauntChains::layout(Chain& chain, const Vector3& start, float whip)
{
    const Vector3 tip = chain.tip;
    Vector3 axis = start - tip;
    const float length = axis.Length();
    axis = length > 0.0001f ? axis * (1.0f / length) : Vector3(0.0f, 0.0f, 1.0f);

    Vector3 side(-axis.z, 0.0f, axis.x);
    if (side.LengthSquared() > 0.0001f)
    {
        side.Normalize();
    }

    const float linkScale = m_owner.m_tauntChainLinkScale;
    const float spacing = m_owner.m_tauntChainLinkSpacing * linkScale;
    const Vector3 park = start + kHiddenOffset;

    // Links are anchored at the tip: they fly out with it, and get "reeled in" into Death as the chain shortens.
    auto linkPosition = [&](float distanceFromTip) -> Vector3
    {
        const float s = length > 0.0001f ? distanceFromTip / length : 0.0f; // 0 = tip, 1 = Death
        const float envelope = std::sin(s * kPi);                             // pinned at both ends
        const float wave = std::sin(s * kPi * 3.0f - m_clock * 22.0f + chain.phase);
        return tip + axis * distanceFromTip + side * (whip * envelope * wave);
    };

    for (size_t i = 0; i < chain.links.size(); ++i)
    {
        Transform* linkTransform = GameObjectAPI::getTransform(chain.links[i]);
        const float distance = (static_cast<float>(i) + 0.5f) * spacing;
        const bool show = distance <= length;

        if (!show)
        {
            if (chain.linkShown[i])
            {
                TransformAPI::setScale(linkTransform, Vector3(kHiddenScale, kHiddenScale, kHiddenScale));
                TransformAPI::setGlobalPosition(linkTransform, park);
                chain.linkShown[i] = false;
            }
            continue;
        }

        if (!chain.linkShown[i])
        {
            // The first link is the one that bites, so it's bigger.
            const float scale = linkScale * (i == 0 ? m_owner.m_tauntChainTipLinkScale : 1.0f);
            TransformAPI::setScale(linkTransform, Vector3(scale, scale, scale));
            chain.linkShown[i] = true;
        }

        const Vector3 position = linkPosition(distance);

        // Face along the (possibly wavy) chain, and turn every other link 90 degrees so they interlock.
        Vector3 direction = linkPosition(distance + spacing * 0.5f) - linkPosition(distance - spacing * 0.5f);
        if (direction.LengthSquared() < 0.000001f)
        {
            direction = axis;
        }
        direction.Normalize();

        const float yaw = std::atan2(direction.x, direction.z) * kRadToDeg;
        const float pitch = -std::asin(direction.y < -1.0f ? -1.0f : (direction.y > 1.0f ? 1.0f : direction.y)) * kRadToDeg;
        const float roll = (i % 2 == 0) ? 0.0f : 90.0f;

        TransformAPI::setGlobalPosition(linkTransform, position);
        TransformAPI::setGlobalRotationEuler(linkTransform, Vector3(pitch, yaw, roll));
    }
}

void DeathTauntChains::hide(Chain& chain, const Vector3& parkPosition)
{
    for (size_t i = 0; i < chain.links.size(); ++i)
    {
        if (!chain.linkShown[i])
        {
            continue;
        }

        Transform* linkTransform = GameObjectAPI::getTransform(chain.links[i]);
        TransformAPI::setScale(linkTransform, Vector3(kHiddenScale, kHiddenScale, kHiddenScale));
        TransformAPI::setGlobalPosition(linkTransform, parkPosition);
        chain.linkShown[i] = false;
    }
}

Vector3 DeathTauntChains::getStart() const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(m_owner.getOwner());
    if (!ownerTransform)
    {
        return Vector3::Zero;
    }

    if (!m_owner.m_tauntChainOnFloor && m_handBone)
    {
        return TransformAPI::getGlobalPosition(m_handBone);
    }

    Vector3 forward = TransformAPI::getForward(ownerTransform);
    forward.y = 0.0f;
    if (forward.LengthSquared() > 0.0001f)
    {
        forward.Normalize();
    }

    const float height = m_owner.m_tauntChainOnFloor ? m_owner.m_tauntChainFloorHeight : 1.3f;
    return TransformAPI::getGlobalPosition(ownerTransform) + forward * m_owner.m_tauntChainStartOffset + Vector3(0.0f, height, 0.0f);
}

// The chain stops at the side of the enemy facing Death, so it hits their front instead of sinking into them.
Vector3 DeathTauntChains::getContactPoint(GameObject* target) const
{
    const Vector3 center = getEnemyCenter(target);

    Vector3 toDeath = getStart() - center;
    toDeath.y = 0.0f;
    const float distance = toDeath.Length();
    if (distance < 0.0001f)
    {
        return center;
    }

    const float maxOffset = distance * 0.8f;
    const float offset = m_owner.m_tauntChainContactOffset < maxOffset ? m_owner.m_tauntChainContactOffset : maxOffset;
    return center + toDeath * (offset / distance);
}

Vector3 DeathTauntChains::getEnemyCenter(GameObject* target) const
{
    Transform* targetTransform = GameObjectAPI::getTransform(target);
    const float height = m_owner.m_tauntChainOnFloor ? m_owner.m_tauntChainFloorHeight : m_owner.m_tauntChainTipHeight;
    return TransformAPI::getGlobalPosition(targetTransform) + Vector3(0.0f, height, 0.0f);
}

bool DeathTauntChains::isTargetValid(GameObject* target) const
{
    if (!target || !SceneAPI::containsGameObject(target))
    {
        return false;
    }

    EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(target);
    return !(damageable && damageable->isDead());
}
