#include "pch.h"
#include "PaladinUI.h"

namespace
{
    constexpr float BasicAttackUIYOffset = 0.05f;
    constexpr float RadiansToDegrees = 57.2957795f;
}

IMPLEMENT_SCRIPT_FIELDS(
    PaladinUI,

    FIELD_GROUP_COLLAPSE(
        "Basic Attack",

        SERIALIZED_COMPONENT_REF(
            m_basicAttackUICanvas,
            "Basic Attack UI Canvas",
            ComponentType::TRANSFORM
        ),

        SERIALIZED_COMPONENT_REF(
            m_basicAttackUIBackground,
            "Basic Attack UI Background",
            ComponentType::TRANSFORM2D
        ),

        SERIALIZED_COMPONENT_REF(
            m_basicAttackUIGlow,
            "Basic Attack UI Glow",
            ComponentType::TRANSFORM2D
        )
    )
)

PaladinUI::PaladinUI(GameObject* owner)
    : Script(owner)
{
}

void PaladinUI::Start()
{
    m_basicAttackUICanvasTransform =
        m_basicAttackUICanvas.getReferencedComponent();

    m_basicAttackUIBackgroundTransform2D =
        m_basicAttackUIBackground.getReferencedComponent();

    m_basicAttackUIGlowTransform2D =
        m_basicAttackUIGlow.getReferencedComponent();

    Transform* ownerTransform =
        GameObjectAPI::getTransform(getOwner());

    if (!m_basicAttackUICanvasTransform && ownerTransform)
    {
        m_basicAttackUICanvasTransform =
            TransformAPI::findChildByName(
                ownerTransform,
                "Paladin Attack UI"
            );
    }

    if (m_basicAttackUICanvasTransform)
    {
        if (!m_basicAttackUIBackgroundTransform2D)
        {
            Transform* backgroundTransform =
                TransformAPI::findChildByName(
                    m_basicAttackUICanvasTransform,
                    "Background"
                );

            if (backgroundTransform)
            {
                GameObject* backgroundObject =
                    ComponentAPI::getOwner(backgroundTransform);

                m_basicAttackUIBackgroundTransform2D =
                    static_cast<Transform2D*>(
                        GameObjectAPI::getComponent(
                            backgroundObject,
                            ComponentType::TRANSFORM2D
                        )
                        );
            }
        }

        if (!m_basicAttackUIGlowTransform2D)
        {
            Transform* glowTransform =
                TransformAPI::findChildByName(
                    m_basicAttackUICanvasTransform,
                    "Glow"
                );

            if (glowTransform)
            {
                GameObject* glowObject =
                    ComponentAPI::getOwner(glowTransform);

                m_basicAttackUIGlowTransform2D =
                    static_cast<Transform2D*>(
                        GameObjectAPI::getComponent(
                            glowObject,
                            ComponentType::TRANSFORM2D
                        )
                        );
            }
        }
    }

    hideBasicAttackUI();
}

void PaladinUI::setupBasicAttackUI(
    float width,
    float length
)
{
    if (!m_basicAttackUIBackgroundTransform2D ||
        !m_basicAttackUIGlowTransform2D)
    {
        return;
    }

    const Vector2 attackAreaScale(width, length);

    Transform2DAPI::setScale(
        m_basicAttackUIBackgroundTransform2D,
        attackAreaScale
    );

    Transform2DAPI::setScale(
        m_basicAttackUIGlowTransform2D,
        attackAreaScale
    );

    Transform2DAPI::setAlpha(
        m_basicAttackUIBackgroundTransform2D,
        0.0f
    );

    Transform2DAPI::setAlpha(
        m_basicAttackUIGlowTransform2D,
        0.0f
    );
}

void PaladinUI::showBasicAttackUI(
    const Vector3& centerPosition,
    const Vector3& forwardDirection
)
{
    if (!m_basicAttackUICanvasTransform ||
        !m_basicAttackUIBackgroundTransform2D ||
        !m_basicAttackUIGlowTransform2D)
    {
        return;
    }

    GameObject* canvasObject =
        ComponentAPI::getOwner(
            m_basicAttackUICanvasTransform
        );

    if (!canvasObject)
    {
        return;
    }

    updateBasicAttackUIPose(
        centerPosition,
        forwardDirection
    );

    Transform2DAPI::setAlpha(
        m_basicAttackUIBackgroundTransform2D,
        0.45f
    );

    Transform2DAPI::setAlpha(
        m_basicAttackUIGlowTransform2D,
        0.0f
    );

    GameObjectAPI::setActive(canvasObject, true);
}

void PaladinUI::updateBasicAttackUIPose(
    const Vector3& centerPosition,
    const Vector3& forwardDirection
)
{
    if (!m_basicAttackUICanvasTransform)
    {
        return;
    }

    Vector3 flatForward = forwardDirection;
    flatForward.y = 0.0f;

    if (flatForward.LengthSquared() < 0.0001f)
    {
        return;
    }

    flatForward.Normalize();

    Vector3 adjustedPosition = centerPosition;
    adjustedPosition.y += BasicAttackUIYOffset;

    const float yawDegrees =
        std::atan2(
            flatForward.x,
            flatForward.z
        ) * RadiansToDegrees;

    const Vector3 canvasRotation(
        90.0f,
        yawDegrees,
        0.0f
    );

    TransformAPI::setGlobalPosition(
        m_basicAttackUICanvasTransform,
        adjustedPosition
    );

    TransformAPI::setGlobalRotationEuler(
        m_basicAttackUICanvasTransform,
        canvasRotation
    );
}

void PaladinUI::showBasicAttackImpact()
{
    hideBasicAttackUI();
}

void PaladinUI::hideBasicAttackUI()
{
    if (!m_basicAttackUICanvasTransform)
    {
        return;
    }

    GameObject* canvasObject =
        ComponentAPI::getOwner(
            m_basicAttackUICanvasTransform
        );

    if (!canvasObject)
    {
        return;
    }

    GameObjectAPI::setActive(canvasObject, false);
}

IMPLEMENT_SCRIPT(PaladinUI)