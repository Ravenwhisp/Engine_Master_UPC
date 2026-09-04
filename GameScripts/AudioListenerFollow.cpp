#include "pch.h"
#include "AudioListenerFollow.h"

IMPLEMENT_SCRIPT(AudioListenerFollow)

IMPLEMENT_SCRIPT_FIELDS(AudioListenerFollow,
    SERIALIZED_COMPONENT_REF(m_firstTarget, "Player 1 (Death)", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_secondTarget, "Player 2 (Lyriel)", ComponentType::TRANSFORM),
    SERIALIZED_VEC3(m_listenerRotation, "Listener Rotation (match camera)")
)

AudioListenerFollow::AudioListenerFollow(GameObject* owner)
    : Script(owner)
{
}

void AudioListenerFollow::Update()
{
    Transform* first  = m_firstTarget.getReferencedComponent();
    Transform* second = m_secondTarget.getReferencedComponent();

    if (!first || !second)
    {
        if (!m_warnedMissing)
        {
            Debug::warn("[AudioListenerFollow] on '%s' is missing one or both player targets.",
                        GameObjectAPI::getName(getOwner()));
            m_warnedMissing = true;
        }
        return;
    }

    Transform* self = GameObjectAPI::getTransform(getOwner());
    if (!self)
    {
        return;
    }

    const Vector3 first2  = TransformAPI::getGlobalPosition(first);
    const Vector3 second2 = TransformAPI::getGlobalPosition(second);
    const Vector3 midpoint = (first2 + second2) * 0.5f;

    TransformAPI::setGlobalPosition(self, midpoint);

    // Fija la orientación cada frame (barato) para que sea live-tweakable desde el
    // inspector mientras ajustas el paneo. La cámara cenital no rota, así que es constante.
    TransformAPI::setGlobalRotationEuler(self, m_listenerRotation);
}
