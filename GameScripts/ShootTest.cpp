#include "pch.h"
#include "ShootTest.h"

IMPLEMENT_SCRIPT_FIELDS(ShootTest,
    SERIALIZED_INT(m_playerIndex, "Player Index"),
    SERIALIZED_FLOAT(m_shotSpeed, "Shot Speed", 0.1f, 100.0f, 0.1f),
    SERIALIZED_FLOAT(m_shotRange, "Shot Range", 0.1f, 100.0f, 0.1f),
    SERIALIZED_FLOAT(m_shotRadius, "Shot Radius", 0.01f, 2.0f, 0.01f)
)

ShootTest::ShootTest(GameObject* owner)
    : Script(owner)
{
}

void ShootTest::Update()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    const Vector3 origin = TransformAPI::getGlobalPosition(ownerTransform);
    const float dt = Time::getDeltaTime();

    if (Input::isRightTriggerJustPressed(m_playerIndex))
    {
        const Vector3 aimDirection = Input::getAimDirection(origin, m_playerIndex);
        if (aimDirection.LengthSquared() > 0.0001f)
        {
            fireShot(origin, aimDirection);
        }
    }

    const Vector3 yellow = { 1.0f, 0.8f, 0.0f };

    for (int i = 0; i < MAX_SHOTS; ++i)
    {
        ShootTestBullet& shot = m_shots[i];
        if (!shot.active)
        {
            continue;
        }

        const Vector3 previousPosition = shot.position;
        const float step = m_shotSpeed * dt;

        shot.position += shot.direction * step;
        shot.traveled += step;

        DebugDrawAPI::drawLine(previousPosition, shot.position, yellow, 0, true);
        DebugDrawAPI::drawSphere(shot.position, yellow, m_shotRadius, 0, true);

        if (shot.traveled >= m_shotRange)
        {
            shot.active = false;
        }
    }
}

void ShootTest::fireShot(const Vector3& origin, const Vector3& direction)
{
    for (int i = 0; i < MAX_SHOTS; ++i)
    {
        if (!m_shots[i].active)
        {
            m_shots[i].active = true;
            m_shots[i].position = origin;
            m_shots[i].direction = direction;
            m_shots[i].traveled = 0.0f;
            return;
        }
    }
}

IMPLEMENT_SCRIPT(ShootTest)
