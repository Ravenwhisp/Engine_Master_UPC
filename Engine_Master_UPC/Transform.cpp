#include "Globals.h"
#include "Transform.h"

const Matrix* Transform::getTransformation()
{
    if (m_dirty)
    {
        calculateMatrix();
    }
    return &m_transformation;
}

void Transform::setRotation(const Quaternion &newRotation)
{
    m_rotation = newRotation;

    m_dirty = true;
}

void Transform::setRotation(const Vector3 &newRotation)
{
    m_rotation = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(newRotation.y), XMConvertToRadians(newRotation.x), XMConvertToRadians(newRotation.z));

    m_dirty = true;
}

void Transform::translate(Vector3* position)
{
    m_position = m_position + *position;
    m_dirty = true;
}

void Transform::rotate(Vector3* eulerAngles)
{
    Quaternion rotation = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(eulerAngles->y), XMConvertToRadians(eulerAngles->x), XMConvertToRadians(eulerAngles->z));

    m_rotation = m_rotation + rotation;

    m_dirty = true;

}

void Transform::rotate(Quaternion* rotation)
{
    m_rotation = m_rotation + *rotation;

    m_dirty = true;
}

void Transform::scalate(Vector3* scale)
{
    m_scale = m_scale + *scale;
    m_dirty = true;
}

const Vector3 Transform::convertQuaternionToEulerAngles(const Quaternion* rotation)
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

void Transform::drawUi() {
    if (ImGui::CollapsingHeader("Transform")) {

        if (ImGui::DragFloat3("Position", &m_position.x, 0.01f))
        {
            m_dirty = true;
        }

        Vector3 rot = convertQuaternionToEulerAngles(&m_rotation);
        if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
        {
            setRotation(rot);
        }

        if (ImGui::DragFloat3("Scale", &m_scale.x, 0.01f))
        {
            m_dirty = true;
        }

        if (ImGui::Button("Reset"))
        {
            m_position = { 0,0,0 };
            m_scale = { 1,1,1 };
            setRotation(Vector3{ 0,0,0 });
        }
    }
}