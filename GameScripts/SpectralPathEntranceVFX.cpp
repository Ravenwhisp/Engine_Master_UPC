#include "pch.h"
#include "SpectralPathEntranceVFX.h"
#include "ObjectVfxIds.h"

SpectralPathEntranceVFX::SpectralPathEntranceVFX(GameObject* owner)
    : Script(owner)
{
}

void SpectralPathEntranceVFX::Start()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    const Vector3 position = ownerTransform != nullptr
        ? TransformAPI::getGlobalPosition(ownerTransform)
        : Vector3::Zero;
    const Vector3 rotation = ownerTransform != nullptr
        ? TransformAPI::getGlobalEulerDegrees(ownerTransform)
        : Vector3::Zero;

    ParticleLifecycle::ensurePersistent(
        m_entranceEffect,
        ObjectVfxIds::spectralPathEntrance(),
        position,
        rotation,
        nullptr
    );

    if (m_entranceEffect != nullptr)
    {
        ParticleLifecycle::activate(m_entranceEffect);
    }
}

void SpectralPathEntranceVFX::OnGameStop()
{
    ParticleLifecycle::destroy(m_entranceEffect);
}

IMPLEMENT_SCRIPT(SpectralPathEntranceVFX)
