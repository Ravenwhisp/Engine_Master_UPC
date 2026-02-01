#include "Transform.h"

const Matrix* Transform::getTransformation()
{
    if (dirty)
    {
        calculateMatrix();
    }
    return &m_transformation;
}

const Transform* Transform::findChild(char* name)
{
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i]->m_gameObject->getName() == name)
        {
            return m_children[i];
        }
    }
    return nullptr;
}

const void Transform::setRotation(Quaternion* newRotation)
{
    m_rotation = *newRotation;

    m_eulerAngles = convertQuaternionToEulerAngles(&m_rotation);

    dirty = true;
}

const void Transform::setRotation(Vector3* newRotation)
{
    m_eulerAngles = *newRotation;

    m_rotation = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(m_eulerAngles.y), XMConvertToRadians(m_eulerAngles.x), XMConvertToRadians(m_eulerAngles.z));

    dirty = true;
}

const void Transform::removeChild(Transform* child)
{
    m_children.erase(std::remove_if(m_children.begin(), m_children.end(), [child](Transform* c)
        {
            if (c == child)
            {
                delete c;
                return true;
            }
            return false;
        }), m_children.end());
}

const void Transform::translate(Vector3* position)
{
    m_position = m_position + *position;
    dirty = true;
}

const void Transform::rotate(Vector3* eulerAngles)
{
    m_eulerAngles = m_eulerAngles + *eulerAngles;

    m_rotation = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(m_eulerAngles.y), XMConvertToRadians(m_eulerAngles.x), XMConvertToRadians(m_eulerAngles.z));

    dirty = true;

}

const void Transform::rotate(Quaternion* rotation)
{
    m_rotation = m_rotation + *rotation;

    m_eulerAngles = convertQuaternionToEulerAngles(&m_rotation);

    dirty = true;
}

const void Transform::scalate(Vector3* scale)
{
    m_scale = m_scale + *scale;
    dirty = true;
}

const Vector3 Transform::convertQuaternionToEulerAngles(Quaternion* rotation)
{
    Quaternion quaternion = *rotation;

    float sinr_cosp = 2.0f * (quaternion.w * quaternion.x + quaternion.y * quaternion.z);
    float cosr_cosp = 1.0f - 2.0f * (quaternion.x * quaternion.x + quaternion.y * quaternion.y);
    float roll = std::atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (quaternion.w * quaternion.y - quaternion.z * quaternion.x);
    float pitch;
    if (std::abs(sinp) >= 1)
    {
        pitch = std::copysign(DirectX::XM_PI / 2, sinp);
    }
    else
    {
        pitch = std::asin(sinp);
    }

    float siny_cosp = 2.0f * (quaternion.w * quaternion.z + quaternion.x * quaternion.y);
    float cosy_cosp = 1.0f - 2.0f * (quaternion.y * quaternion.y + quaternion.z * quaternion.z);

    float yaw = std::atan2(siny_cosp, cosy_cosp);

    return Vector3(pitch, roll, yaw);
}

const void Transform::calculateMatrix()
{
    m_transformation = Matrix::CreateScale(m_scale) * Matrix::CreateFromQuaternion(m_rotation) * Matrix::CreateTranslation(m_position);

}