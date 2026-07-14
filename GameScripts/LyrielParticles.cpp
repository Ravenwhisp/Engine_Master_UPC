#include "pch.h"
#include "LyrielParticles.h"


IMPLEMENT_SCRIPT_FIELDS(LyrielParticles,
    SERIALIZED_COMPONENT_REF(m_dashTrail, "Dash", ComponentType::TRANSFORM)
)

LyrielParticles::LyrielParticles(GameObject* owner) : Script(owner)
{
}

Transform* LyrielParticles::getTransform(ComponentRef<Transform> controller)
{
    Transform* particleTransform = controller.getReferencedComponent();

    if (particleTransform == nullptr)
    {
        Debug::warn("Missing reference on Lyriel Particles on %s.", GameObjectAPI::getName(getOwner()));
        return nullptr;
    }

    return particleTransform;

}

void LyrielParticles::SetDashActive()
{
    if (m_dashTrailController == nullptr)
    {
        m_dashTrailController = getTransform(m_dashTrail);

        if (m_dashTrailController == nullptr)
        {
            Debug::warn("Dash trail controller not found on Lyriel Particles.");
            return;
        }

    }

    int childCount = TransformAPI::getChildCount(m_dashTrailController);

    for (int i = 0; i < childCount; i++)
    {
        Transform* child = TransformAPI::getChild(m_dashTrailController, i);

        TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(child));
        TrailAPI::generateTrail(trailComponent, true);
    }
}

void LyrielParticles::SetDashInactive()
{
    if (m_dashTrailController == nullptr)
    {
        m_dashTrailController = getTransform(m_dashTrail);

        if (m_dashTrailController == nullptr)
        {
            Debug::warn("Dash trail controller not found on Lyriel Particles.");
            return;
        }

    }

    int childCount = TransformAPI::getChildCount(m_dashTrailController);

    for (int i = 0; i < childCount; i++)
    {
        Transform* child = TransformAPI::getChild(m_dashTrailController, i);

        TrailComponent* trailComponent = TrailAPI::getTrailComponent(ComponentAPI::getOwner(child));
        TrailAPI::generateTrail(trailComponent, false);
    }
}

IMPLEMENT_SCRIPT(LyrielParticles)