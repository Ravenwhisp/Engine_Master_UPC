#include "pch.h"
#include "BreakableExplosive.h"

#include "Transform2D.h"
#include "Damageable.h"
#include "EnvironmentSound.h"
#include "ObjectVfxIds.h"
#include "ParticleLifecycle.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(BreakableExplosive, BreakableObject,
    SERIALIZED_FLOAT(m_explosionRadius, "Explosion Radius", 0.0f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_explosionDamage, "Explosion Damage", 0.0f, 100.0f, 1.0f),
    SERIALIZED_FLOAT(m_telegraphRevealDistance, "Telegraph Reveal Distance", 0.0f, 50.0f, 0.5f),
    SERIALIZED_FLOAT(m_telegraphHideDistance, "Telegraph Hide Distance", 0.0f, 50.0f, 0.5f),
    SERIALIZED_FLOAT(m_telegraphFadeDuration, "Telegraph Fade Duration", 0.0f, 2.0f, 0.05f),
    SERIALIZED_FLOAT(m_telegraphMaxAlpha, "Telegraph Max Alpha", 0.0f, 1.0f, 0.05f),
    SERIALIZED_FLOAT(m_telegraphHeightOffset, "Telegraph Height Offset", -1.0f, 1.0f, 0.01f),
    SERIALIZED_COMPONENT_REF(m_telegraphCanvas, "Telegraph Canvas", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_telegraphVisual, "Telegraph Visual", ComponentType::TRANSFORM2D)
)

BreakableExplosive::BreakableExplosive(GameObject* owner)
    : BreakableObject(owner)
{
}

void BreakableExplosive::Start()
{
    BreakableObject::Start();

    m_ownerTransform = GameObjectAPI::getTransform(getOwner());
    m_players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    setupTelegraph();
}

void BreakableExplosive::Update()
{
    BreakableObject::Update();

    if (isBroken())
    {
        return;
    }

    updateTelegraph(Time::getDeltaTime());
}

void BreakableExplosive::setupTelegraph()
{
    if (m_ownerTransform == nullptr)
    {
        Debug::warn("[BreakableExplosive] '%s' has no owner Transform.", GameObjectAPI::getName(getOwner()));
        return;
    }

    // Find ExplosionTelegraphCanvas under the barrel.
    m_telegraphCanvasTransform = TransformAPI::findChildByName(m_ownerTransform, "ExplosionTelegraphCanvas");

    if (m_telegraphCanvasTransform == nullptr)
    {
        Debug::warn("[BreakableExplosive] '%s' could not find child '%s'.", GameObjectAPI::getName(getOwner()), "ExplosionTelegraphCanvas");
        return;
    }

    m_telegraphCanvasObject = ComponentAPI::getOwner(m_telegraphCanvasTransform);

    if (m_telegraphCanvasObject == nullptr)
    {
        return;
    }

    // Find ExplosionTelegraphSprite under the canvas.
    Transform* spriteTransform = TransformAPI::findChildByName(m_telegraphCanvasTransform, "ExplosionTelegraphSprite");

    if (spriteTransform == nullptr)
    {
        Debug::warn("[BreakableExplosive] '%s' could not find '%s/%s'.", GameObjectAPI::getName(getOwner()), "ExplosionTelegraphCanvas", "ExplosionTelegraphSprite");
        return;
    }

    GameObject* spriteObject = ComponentAPI::getOwner(spriteTransform);

    if (spriteObject == nullptr)
    {
        return;
    }

    Component* visualComponent = GameObjectAPI::getComponent(spriteObject, ComponentType::TRANSFORM2D);
    m_telegraphVisualTransform = static_cast<Transform2D*>(visualComponent);

    if (m_telegraphVisualTransform == nullptr)
    {
        Debug::warn("[BreakableExplosive] '%s/%s' requires a Transform2D component.", "ExplosionTelegraphCanvas", "ExplosionTelegraphSprite");
        return;
    }

    GameObjectAPI::setActive(m_telegraphCanvasObject, true);

    Vector3 telegraphPosition = TransformAPI::getGlobalPosition(m_ownerTransform);
    telegraphPosition.y += m_telegraphHeightOffset;

    TransformAPI::setGlobalPosition(m_telegraphCanvasTransform, telegraphPosition);
    TransformAPI::setGlobalRotationEuler(m_telegraphCanvasTransform, Vector3(90.0f, 0.0f, 0.0f));

    const float baseDiameter = Transform2DAPI::getBaseSize(m_telegraphVisualTransform).x;

    if (baseDiameter > 0.001f)
    {
        const float desiredDiameter = m_explosionRadius * 2.0f * 100.0f;
        const float scale = desiredDiameter / baseDiameter;
        Transform2DAPI::setScale(m_telegraphVisualTransform, Vector2(scale, scale));
    }

    m_currentTelegraphAlpha = 0.0f;
    m_telegraphShouldShow = false;

    applyTelegraphAlpha(0.0f);
    GameObjectAPI::setActive(m_telegraphCanvasObject, false);
}

void BreakableExplosive::updateTelegraph(float deltaTime)
{
    if (m_telegraphCanvasObject == nullptr || m_telegraphVisualTransform == nullptr)
    {
        return;
    }

    const float revealDistance = (std::max)(m_telegraphRevealDistance, 0.0f);
    const float hideDistance = (std::max)(m_telegraphHideDistance, revealDistance);
    const float testDistance = m_telegraphShouldShow ? hideDistance : revealDistance;
    m_telegraphShouldShow = isAnyPlayerWithin(testDistance);

    const float maxAlpha = std::clamp(m_telegraphMaxAlpha, 0.0f, 1.0f);
    const float targetAlpha = m_telegraphShouldShow ? maxAlpha : 0.0f;

    if (targetAlpha > 0.0f && !GameObjectAPI::isActiveSelf(m_telegraphCanvasObject))
    {
        GameObjectAPI::setActive(m_telegraphCanvasObject, true);
    }

    if (m_telegraphFadeDuration <= 0.0f)
    {
        m_currentTelegraphAlpha = targetAlpha;
    }
    else
    {
        const float fadeSpeed = maxAlpha / m_telegraphFadeDuration;
        m_currentTelegraphAlpha = MathAPI::moveTowards(m_currentTelegraphAlpha, targetAlpha, fadeSpeed * deltaTime);
    }

    applyTelegraphAlpha(m_currentTelegraphAlpha);

    if (targetAlpha <= 0.0f && m_currentTelegraphAlpha <= 0.0f)
    {
        GameObjectAPI::setActive(m_telegraphCanvasObject, false);
    }
}

bool BreakableExplosive::isAnyPlayerWithin(float distance) const
{
    if (m_ownerTransform == nullptr)
    {
        return false;
    }

    const Vector3 barrelPosition = TransformAPI::getGlobalPosition(m_ownerTransform);
    const float distanceSquared = distance * distance;

    for (GameObject* player : m_players)
    {
        if (player == nullptr || !GameObjectAPI::isActiveInHierarchy(player))
        {
            continue;
        }

        Transform* playerTransform = GameObjectAPI::getTransform(player);
        if (playerTransform == nullptr)
        {
            continue;
        }

        const Vector3 playerPosition = TransformAPI::getGlobalPosition(playerTransform);
        const float dx = playerPosition.x - barrelPosition.x;
        const float dz = playerPosition.z - barrelPosition.z;

        if ((dx * dx) + (dz * dz) <= distanceSquared)
        {
            return true;
        }
    }

    return false;
}

void BreakableExplosive::applyTelegraphAlpha(float alpha)
{
    if (m_telegraphVisualTransform != nullptr)
    {
        Transform2DAPI::setAlpha(m_telegraphVisualTransform, alpha);
    }
}

void BreakableExplosive::hideTelegraphImmediately()
{
    m_telegraphShouldShow = false;
    m_currentTelegraphAlpha = 0.0f;
    applyTelegraphAlpha(0.0f);

    if (m_telegraphCanvasObject != nullptr)
    {
        GameObjectAPI::setActive(m_telegraphCanvasObject, false);
    }
}

void BreakableExplosive::drawGizmo()
{
    if (m_ownerTransform == nullptr)
    {
        m_ownerTransform = GameObjectAPI::getTransform(getOwner());
    }

    if (m_ownerTransform == nullptr)
    {
        return;
    }

    const Vector3 position = TransformAPI::getGlobalPosition(m_ownerTransform);

    DebugDrawAPI::drawCircle(position, Vector3::UnitY, Vector3(1.0f, 0.0f, 0.0f), m_explosionRadius);
    DebugDrawAPI::drawCircle(position, Vector3::UnitY, Vector3(1.0f, 0.65f, 0.0f), m_telegraphRevealDistance);
}

void BreakableExplosive::onBreak()
{
    hideTelegraphImmediately();

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr)
    {
        return;
    }

    const Vector3 explosionCenter = TransformAPI::getGlobalPosition(ownerTransform);
    const float explosionRadiusSquared = m_explosionRadius * m_explosionRadius;

    const std::vector<GameObject*> objectsInArea = SceneAPI::getObjectsInCircularArea(Vector2(explosionCenter.x, explosionCenter.z), m_explosionRadius);

    for (GameObject* go : objectsInArea)
    {
        Damageable* damageableScript = GameObjectAPI::findScript<Damageable>(go);
        if (damageableScript == nullptr)
        {
            continue;
        }

        Transform* targetTransform = GameObjectAPI::getTransform(go);
        if (targetTransform == nullptr)
        {
            continue;
        }

        const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);
        if (Vector3::DistanceSquared(explosionCenter, targetPosition) > explosionRadiusSquared)
        {
            continue;
        }

        damageableScript->takeDamage(m_explosionDamage);
    }

    ParticleLifecycle::spawnOneShotTimed(m_timedBreakEffects, ObjectVfxIds::barrelExplosion(), getBreakEffectPosition(), Vector3::Zero, ParticleLifecycle::kDefaultOneShotLifetime, getOwner());

    EnvironmentSound::play(getOwner(), "Play_Environment_Explosive_Barrel");

    BreakableObject::breakObject();
}

IMPLEMENT_SCRIPT(BreakableExplosive)
