#include "pch.h"
#include "PaladinUI.h"

namespace
{
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
            m_basicAttackUITelegraph,
            "Basic Attack UI Telegraph",
            ComponentType::TRANSFORM2D
        ),

        SERIALIZED_COMPONENT_REF(
            m_basicAttackUIImpact,
            "Basic Attack UI Impact",
            ComponentType::TRANSFORM2D
        ),

        SERIALIZED_FLOAT(m_basicAttackUIWidthMultiplier, "Width Multiplier", 0.1f, 5.0f, 0.01f),
        SERIALIZED_FLOAT(m_basicAttackUILengthMultiplier, "Length Multiplier", 0.1f, 5.0f, 0.01f),

        SERIALIZED_FLOAT(m_basicAttackUIForwardOffset, "Forward Offset", -5.0f, 5.0f, 0.01f),
        SERIALIZED_FLOAT(m_basicAttackUISideOffset, "Side Offset", -5.0f, 5.0f, 0.01f),
        SERIALIZED_FLOAT(m_basicAttackUIHeightOffset, "Height Offset", 0.0f, 1.0f, 0.01f),

        SERIALIZED_FLOAT(m_basicAttackUITelegraphAlpha, "Telegraph Alpha", 0.0f, 1.0f, 0.01f),
        SERIALIZED_FLOAT(m_basicAttackUIImpactAlpha, "Impact Alpha", 0.0f, 1.0f, 0.01f)
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

    m_basicAttackUITelegraphTransform2D =
        m_basicAttackUITelegraph.getReferencedComponent();

    m_basicAttackUIImpactTransform2D =
        m_basicAttackUIImpact.getReferencedComponent();

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
        if (!m_basicAttackUITelegraphTransform2D)
        {
            Transform* telegraphTransform =
                TransformAPI::findChildByName(
                    m_basicAttackUICanvasTransform,
                    "Telegraph"
                );

            if (telegraphTransform)
            {
                GameObject* telegraphObject =
                    ComponentAPI::getOwner(telegraphTransform);

                m_basicAttackUITelegraphTransform2D =
                    static_cast<Transform2D*>(
                        GameObjectAPI::getComponent(
                            telegraphObject,
                            ComponentType::TRANSFORM2D
                        )
                        );
            }
        }

        if (!m_basicAttackUIImpactTransform2D)
        {
            Transform* impactTransform =
                TransformAPI::findChildByName(
                    m_basicAttackUICanvasTransform,
                    "Impact"
                );

            if (impactTransform)
            {
                GameObject* impactObject =
                    ComponentAPI::getOwner(impactTransform);

                m_basicAttackUIImpactTransform2D =
                    static_cast<Transform2D*>(
                        GameObjectAPI::getComponent(
                            impactObject,
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
    const Vector2 attackAreaScale(
        width * m_basicAttackUIWidthMultiplier,
        length * m_basicAttackUILengthMultiplier
    );

    if (m_basicAttackUITelegraphTransform2D)
    {
        Transform2DAPI::setScale(
            m_basicAttackUITelegraphTransform2D,
            attackAreaScale
        );

        Transform2DAPI::setAlpha(
            m_basicAttackUITelegraphTransform2D,
            0.0f
        );
    }

    if (m_basicAttackUIImpactTransform2D)
    {
        Transform2DAPI::setScale(
            m_basicAttackUIImpactTransform2D,
            attackAreaScale
        );

        Transform2DAPI::setAlpha(
            m_basicAttackUIImpactTransform2D,
            0.0f
        );
    }
}

void PaladinUI::showBasicAttackUI(
    const Vector3& centerPosition,
    const Vector3& forwardDirection
)
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

    updateBasicAttackUIPose(
        centerPosition,
        forwardDirection
    );

    if (m_basicAttackUITelegraphTransform2D)
    {
        Transform2DAPI::setAlpha(
            m_basicAttackUITelegraphTransform2D,
            m_basicAttackUITelegraphAlpha
        );
    }

    if (m_basicAttackUIImpactTransform2D)
    {
        Transform2DAPI::setAlpha(
            m_basicAttackUIImpactTransform2D,
            0.0f
        );
    }

    GameObjectAPI::setActive(
        canvasObject,
        true
    );
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

    Vector3 right(
        -flatForward.z,
        0.0f,
        flatForward.x
    );

    right.Normalize();

    Vector3 adjustedPosition = centerPosition;
    adjustedPosition += flatForward * m_basicAttackUIForwardOffset;
    adjustedPosition += right * m_basicAttackUISideOffset;
    adjustedPosition.y += m_basicAttackUIHeightOffset;

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

    GameObjectAPI::setActive(
        canvasObject,
        true
    );

    if (m_basicAttackUITelegraphTransform2D)
    {
        Transform2DAPI::setAlpha(
            m_basicAttackUITelegraphTransform2D,
            0.0f
        );
    }

    if (m_basicAttackUIImpactTransform2D)
    {
        Transform2DAPI::setAlpha(
            m_basicAttackUIImpactTransform2D,
            m_basicAttackUIImpactAlpha
        );
    }
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

    GameObjectAPI::setActive(
        canvasObject,
        false
    );
}

IMPLEMENT_SCRIPT(PaladinUI)