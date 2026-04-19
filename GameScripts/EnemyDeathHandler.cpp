#include "pch.h"
#include "EnemyDeathHandler.h"
#include "Damageable.h"

static const ScriptFieldInfo EnemyDeathHandlerFields[] =
{
    { "Destroy Delay", ScriptFieldType::Float, offsetof(EnemyDeathHandler, m_destroyDelay), { 0.0f, 30.0f, 0.1f } },
    { "Death State Name", ScriptFieldType::String, offsetof(EnemyDeathHandler, m_deathStateName) }
};

IMPLEMENT_SCRIPT_FIELDS(EnemyDeathHandler, EnemyDeathHandlerFields)

EnemyDeathHandler::EnemyDeathHandler(GameObject* owner)
    : Script(owner)
{
}

void EnemyDeathHandler::Start()
{
    Script* script = GameObjectAPI::getScript(m_owner, "Damageable");
    m_damageable = dynamic_cast<Damageable*>(script);

    if (m_damageable == nullptr)
    {
        Debug::warn("EnemyDeathHandler on '%s' could not find Damageable.", GameObjectAPI::getName(m_owner));
    }
}

void EnemyDeathHandler::Update()
{
    if (m_damageable == nullptr)
    {
        return;
    }

    if (!m_hasProcessedDeath && m_damageable->isDead())
    {
        processDeath();
    }

    if (m_waitingToDestroy)
    {
        m_deathTimer -= Time::getDeltaTime();

        if (m_deathTimer <= 0.0f)
        {
            destroyEnemyNow();
        }
    }
}

void EnemyDeathHandler::processDeath()
{
    if (m_hasProcessedDeath)
    {
        return;
    }

    m_hasProcessedDeath = true;

    playDeathAnimation();
    startDestroyCountdown(m_destroyDelay);
}

void EnemyDeathHandler::playDeathAnimation()
{
    if (m_deathStateName.empty())
    {
        return;
    }

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(m_owner);

    if (animation == nullptr)
    {
        Debug::warn("EnemyDeathHandler on '%s' could not find AnimationComponent.", GameObjectAPI::getName(m_owner));
        return;
    }

    const bool played = AnimationAPI::playState(animation, m_deathStateName.c_str(), 0.0f);

    if (!played)
    {
        Debug::warn("EnemyDeathHandler on '%s' could not play death state '%s'.", GameObjectAPI::getName(m_owner), m_deathStateName.c_str());
    }
}

void EnemyDeathHandler::startDestroyCountdown(float delay)
{
    m_waitingToDestroy = true;
    m_deathTimer = delay;
}

void EnemyDeathHandler::destroyEnemyNow()
{
    m_waitingToDestroy = false;
    GameObjectAPI::removeGameObject(m_owner);
}


IMPLEMENT_SCRIPT(EnemyDeathHandler)