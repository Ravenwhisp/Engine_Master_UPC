#include "pch.h"
#include "DestroyParticles.h"

IMPLEMENT_SCRIPT_FIELDS(DestroyParticles,
    SERIALIZED_FLOAT(m_lifetime, "Lifetime", 0.0f, 60.0f, 0.1f)
)

DestroyParticles::DestroyParticles(GameObject* owner)
    : Script(owner)
{
}

void DestroyParticles::Start()
{
    m_timer = 0.0f;
    Debug::log("[DestroyParticles] '%s' will be destroyed in %.2f seconds.", GameObjectAPI::getName(getOwner()), m_lifetime);
}

void DestroyParticles::Update()
{
    m_timer += Time::getDeltaTime();

    Debug::log("[DestroyParticles] '%s' — %.2f s remaining.", GameObjectAPI::getName(getOwner()), m_lifetime - m_timer);

    if (m_timer >= m_lifetime)
    {
        GameObjectAPI::removeGameObject(getOwner());
    }
}

IMPLEMENT_SCRIPT(DestroyParticles)
