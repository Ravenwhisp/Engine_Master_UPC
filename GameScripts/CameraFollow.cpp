#include "pch.h"
#include "CameraFollow.h"
#include "CameraShake.h"

IMPLEMENT_SCRIPT_FIELDS(CameraFollow,
    SERIALIZED_COMPONENT_REF(
        m_firstTarget,
        "First Target",
        ComponentType::TRANSFORM
    ),
    SERIALIZED_COMPONENT_REF(
        m_secondTarget,
        "Second Target",
        ComponentType::TRANSFORM
    ),
    SERIALIZED_VEC3(
        m_transformOffset,
        "World Offset"
    ),
    SERIALIZED_VEC3(
        m_rotationOffset,
        "Fixed Rotation"
    ),
    SERIALIZED_FLOAT(
        m_followSharpness,
        "Follow Sharpness",
        0.0f,
        50.0f,
        0.1f
    ),
    SERIALIZED_FLOAT(
        m_zoomSharpness,
        "Zoom Sharpness",
        0.0f,
        50.0f,
        0.1f
    ),
    SERIALIZED_FLOAT(
        m_zoomStartDistance,
        "Zoom Start Distance",
        0.0f,
        1000.0f,
        0.05f
    ),
    SERIALIZED_FLOAT(
        m_zoomEndDistance,
        "Zoom End Distance",
        0.0f,
        1000.0f,
        0.05f
    ),
    SERIALIZED_FLOAT(
        m_maxExtraHeight,
        "Max Extra Height",
        0.0f,
        1000.0f,
        0.05f
    )
)

CameraFollow::CameraFollow(GameObject* owner)
    : Script(owner)
{
}

void CameraFollow::Start()
{
    m_cameraShake = GameObjectAPI::findScript<CameraShake>(getOwner());
}

Vector3 CameraFollow::currentShakeOffset() const
{
    if (m_cameraShake == nullptr)
    {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    return m_cameraShake->getCurrentOffset();
}

void CameraFollow::beginVerticalOnlyFollow(Transform* anchor)
{
    if (anchor == nullptr)
    {
        Debug::warn(
            "CameraFollow could not begin vertical-only follow because the anchor is null."
        );
        return;
    }

    Transform* firstTarget =
        m_firstTarget.getReferencedComponent();

    if (firstTarget == nullptr)
    {
        Debug::warn(
            "CameraFollow could not begin vertical-only follow because the first target is null."
        );
        return;
    }

    Transform* secondTarget =
        m_secondTarget.getReferencedComponent();

    float highestTargetY =
        TransformAPI::getGlobalPosition(firstTarget).y;

    if (secondTarget != nullptr)
    {
        const float secondTargetY =
            TransformAPI::getGlobalPosition(secondTarget).y;

        if (secondTargetY > highestTargetY)
        {
            highestTargetY = secondTargetY;
        }
    }


    m_verticalFollowStartTargetY = highestTargetY;

    m_verticalFollowAnchor = anchor;
    m_verticalOnlyFollowActive = true;


    m_currentExtraHeight = 0.0f;

    Debug::log(
        "CameraFollow: vertical-only follow enabled. Start target Y = %.2f",
        m_verticalFollowStartTargetY
    );
}

void CameraFollow::endVerticalOnlyFollow()
{
    m_verticalOnlyFollowActive = false;
    m_verticalFollowAnchor = nullptr;
    m_verticalFollowStartTargetY = 0.0f;

    m_currentExtraHeight = 0.0f;

    Debug::log(
        "CameraFollow: vertical-only follow disabled."
    );
}

void CameraFollow::Update()
{
    static int s_frameCount = 0;
    const bool isEarlyFrame = (s_frameCount < 10);

    if (!m_followEnabled)
    {
        if (isEarlyFrame)
        {
            Debug::log(
                "[CameraFollow DIAG] Frame %d: m_followEnabled=false, skipping.",
                s_frameCount
            );
        }

        ++s_frameCount;
        return;
    }

    Transform* firstTarget =
        m_firstTarget.getReferencedComponent();

    if (!firstTarget)
    {
        if (isEarlyFrame)
        {
            Debug::log(
                "[CameraFollow DIAG] Frame %d: m_firstTarget is null, skipping.",
                s_frameCount
            );
        }

        ++s_frameCount;
        return;
    }

    GameObject* camera = getOwner();

    Transform* cameraTransform =
        GameObjectAPI::getTransform(camera);

    Transform* secondTarget =
        m_secondTarget.getReferencedComponent();

    const bool hasSecondTarget =
        (secondTarget != nullptr);

    const float dt =
        (std::min)(Time::getDeltaTime(), 0.05f);

    if (isEarlyFrame)
    {
        const Vector3 camPos =
            TransformAPI::getGlobalPosition(cameraTransform);

        const Vector3 targetPos =
            TransformAPI::getGlobalPosition(firstTarget);

        const float dist =
            (targetPos - camPos).Length();

        Debug::log(
            "[CameraFollow DIAG] Frame %d: dt=%.4f cam=(%.1f,%.1f,%.1f) "
            "target=(%.1f,%.1f,%.1f) dist=%.1f firstUpdate=%d",
            s_frameCount,
            dt,
            camPos.x,
            camPos.y,
            camPos.z,
            targetPos.x,
            targetPos.y,
            targetPos.z,
            dist,
            m_firstUpdateAfterResolve ? 1 : 0
        );
    }

    ++s_frameCount;


    // VERTICAL-ONLY FOLLOW

    if (
        m_verticalOnlyFollowActive &&
        m_verticalFollowAnchor != nullptr
        )
    {
        const Vector3 anchorPosition =
            TransformAPI::getGlobalPosition(
                m_verticalFollowAnchor
            );

        const Vector3 anchorRotation =
            TransformAPI::getGlobalEulerDegrees(
                m_verticalFollowAnchor
            );

        float highestTargetY =
            TransformAPI::getGlobalPosition(
                firstTarget
            ).y;

        if (secondTarget != nullptr)
        {
            const float secondTargetY =
                TransformAPI::getGlobalPosition(
                    secondTarget
                ).y;

            if (secondTargetY > highestTargetY)
            {
                highestTargetY = secondTargetY;
            }
        }


        const float verticalDelta =
            highestTargetY -
            m_verticalFollowStartTargetY;

        Vector3 desiredPosition =
            anchorPosition;


        desiredPosition.y =
            anchorPosition.y +
            verticalDelta;

        const Vector3 currentPosition =
            TransformAPI::getGlobalPosition(
                cameraTransform
            );

        const Vector3 smoothedPosition =
            smoothCameraPosition(
                currentPosition,
                desiredPosition,
                m_followSharpness,
                dt
            );

        TransformAPI::setGlobalRotationEuler(
            cameraTransform,
            anchorRotation
        );

        TransformAPI::setGlobalPosition(
            cameraTransform,
            smoothedPosition + currentShakeOffset()
        );

        m_firstUpdateAfterResolve = false;
        return;
    }

 
    // ORIGINAL CAMERA FOLLOW


    TransformAPI::setGlobalRotationEuler(
        cameraTransform,
        m_rotationOffset
    );

    Vector3 followPoint =
        computeFollowPoint();

    float targetExtraHeight = 0.0f;

    if (hasSecondTarget)
    {
        const Vector3 p1 =
            TransformAPI::getGlobalPosition(
                firstTarget
            );

        const Vector3 p2 =
            TransformAPI::getGlobalPosition(
                secondTarget
            );

        targetExtraHeight =
            computeTargetExtraHeight(
                p1,
                p2
            );
    }

    m_currentExtraHeight =
        smoothExtraHeight(
            m_currentExtraHeight,
            targetExtraHeight,
            m_zoomSharpness,
            dt
        );

    const Vector3 desiredPos =
        computeDesiredCameraPosition(
            followPoint,
            cameraTransform
        );

    if (m_firstUpdateAfterResolve)
    {
        TransformAPI::setGlobalPosition(
            cameraTransform,
            desiredPos
        );

        TransformAPI::setGlobalRotationEuler(
            cameraTransform,
            m_rotationOffset
        );

        m_firstUpdateAfterResolve = false;
        return;
    }

    const Vector3 currentPos =
        TransformAPI::getGlobalPosition(
            cameraTransform
        );

    const Vector3 smoothedCameraPosition =
        smoothCameraPosition(
            currentPos,
            desiredPos,
            m_followSharpness,
            dt
        );

    TransformAPI::setGlobalPosition(
        cameraTransform,
        smoothedCameraPosition + currentShakeOffset()
    );
}

bool CameraFollow::getDesiredCameraTransform(
    Vector3& outPosition,
    Vector3& outRotation
)
{
    Transform* firstTarget =
        m_firstTarget.getReferencedComponent();

    if (firstTarget == nullptr)
    {
        return false;
    }

    Transform* secondTarget =
        m_secondTarget.getReferencedComponent();

    // VERTICAL-ONLY FOLLOW DESIRED TRANSFORM

    if (
        m_verticalOnlyFollowActive &&
        m_verticalFollowAnchor != nullptr
        )
    {
        const Vector3 anchorPosition =
            TransformAPI::getGlobalPosition(
                m_verticalFollowAnchor
            );

        float highestTargetY =
            TransformAPI::getGlobalPosition(
                firstTarget
            ).y;

        if (secondTarget != nullptr)
        {
            const float secondTargetY =
                TransformAPI::getGlobalPosition(
                    secondTarget
                ).y;

            if (secondTargetY > highestTargetY)
            {
                highestTargetY =
                    secondTargetY;
            }
        }

        const float verticalDelta =
            highestTargetY -
            m_verticalFollowStartTargetY;

        outPosition =
            anchorPosition;

        outPosition.y =
            anchorPosition.y +
            verticalDelta;

        outRotation =
            TransformAPI::getGlobalEulerDegrees(
                m_verticalFollowAnchor
            );

        return true;
    }



    GameObject* camera =
        getOwner();

    Transform* cameraTransform =
        GameObjectAPI::getTransform(
            camera
        );

    const bool hasSecondTarget =
        secondTarget != nullptr;

    const float dt =
        (std::min)(
            Time::getDeltaTime(),
            0.05f
            );

    Vector3 followPoint =
        computeFollowPoint();

    float targetExtraHeight =
        0.0f;

    if (hasSecondTarget)
    {
        const Vector3 p1 =
            TransformAPI::getPosition(
                firstTarget
            );

        const Vector3 p2 =
            TransformAPI::getPosition(
                secondTarget
            );

        targetExtraHeight =
            computeTargetExtraHeight(
                p1,
                p2
            );
    }

    m_currentExtraHeight =
        smoothExtraHeight(
            m_currentExtraHeight,
            targetExtraHeight,
            m_zoomSharpness,
            dt
        );

    const Vector3 previousRotation =
        TransformAPI::getEulerDegrees(
            cameraTransform
        );

    TransformAPI::setRotationEuler(
        cameraTransform,
        m_rotationOffset
    );

    outPosition =
        computeDesiredCameraPosition(
            followPoint,
            cameraTransform
        );

    outRotation =
        m_rotationOffset;

    TransformAPI::setRotationEuler(
        cameraTransform,
        previousRotation
    );

    return true;
}

Vector3 CameraFollow::computeFollowPoint() const
{
    Transform* firstTarget =
        m_firstTarget.getReferencedComponent();

    Transform* secondTarget =
        m_secondTarget.getReferencedComponent();

    if (!secondTarget)
    {
        return TransformAPI::getGlobalPosition(
            firstTarget
        );
    }

    const Vector3 p1 =
        TransformAPI::getGlobalPosition(
            firstTarget
        );

    const Vector3 p2 =
        TransformAPI::getGlobalPosition(
            secondTarget
        );

    return (p1 + p2) * 0.5f;
}

float CameraFollow::computeTargetExtraHeight(
    const Vector3& p1,
    const Vector3& p2
) const
{
    const float distance =
        (p2 - p1).Length();

    const float zoomRange =
        m_zoomEndDistance -
        m_zoomStartDistance;

    const float distancePastZoomStart =
        distance -
        m_zoomStartDistance;

    float normalizedZoomFactor =
        0.0f;

    if (zoomRange > 0.0001f)
    {
        normalizedZoomFactor =
            distancePastZoomStart /
            zoomRange;

        if (normalizedZoomFactor < 0.0f)
        {
            normalizedZoomFactor =
                0.0f;
        }

        if (normalizedZoomFactor > 1.0f)
        {
            normalizedZoomFactor =
                1.0f;
        }
    }

    return
        m_maxExtraHeight *
        normalizedZoomFactor;
}

float CameraFollow::smoothExtraHeight(
    float current,
    float target,
    float sharpness,
    float dt
) const
{
    if (sharpness <= 0.0f)
    {
        return target;
    }

    const float zoomFraction =
        1.0f -
        expf(-sharpness * dt);

    return lerpFloat(
        current,
        target,
        zoomFraction
    );
}

Vector3 CameraFollow::computeDesiredCameraPosition(
    const Vector3& followPoint,
    Transform* const& cameraTransform
) const
{
    Transform* firstTarget =
        m_firstTarget.getReferencedComponent();

    Transform* secondTarget =
        m_secondTarget.getReferencedComponent();

    Vector3 desiredPos =
        followPoint;

    desiredPos.x +=
        m_transformOffset.x;

    desiredPos.z +=
        m_transformOffset.z;

    float highestTargetY =
        TransformAPI::getGlobalPosition(
            firstTarget
        ).y;

    if (secondTarget)
    {
        const float secondTargetY =
            TransformAPI::getGlobalPosition(
                secondTarget
            ).y;

        if (secondTargetY > highestTargetY)
        {
            highestTargetY =
                secondTargetY;
        }
    }

    desiredPos.y =
        highestTargetY +
        m_transformOffset.y;

    Vector3 forward =
        TransformAPI::getForward(
            cameraTransform
        );

    forward.Normalize();

    desiredPos -=
        forward *
        m_currentExtraHeight;

    return desiredPos;
}

Vector3 CameraFollow::smoothCameraPosition(
    const Vector3& current,
    const Vector3& target,
    float sharpness,
    float dt
) const
{
    if (sharpness <= 0.0f)
    {
        return target;
    }

    const float followFraction =
        1.0f -
        expf(-sharpness * dt);

    return lerpVector(
        current,
        target,
        followFraction
    );
}

Vector3 CameraFollow::lerpVector(
    const Vector3& start,
    const Vector3& end,
    float alpha
) const
{
    return
        start +
        (end - start) *
        alpha;
}

float CameraFollow::lerpFloat(
    float start,
    float end,
    float alpha
) const
{
    return
        start +
        (end - start) *
        alpha;
}

IMPLEMENT_SCRIPT(CameraFollow)