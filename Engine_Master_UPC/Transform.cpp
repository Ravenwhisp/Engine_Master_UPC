#include "Globals.h"
#include "Transform.h"
#include "GameObject.h"
#include <cmath>

Transform::Transform(int id, GameObject* gameObject) :
    Component(id, ComponentType::TRANSFORM, gameObject),
    m_dirty(true),
    m_root(nullptr),
    m_globalMatrix(Matrix::Identity),
    m_position(Vector3::Zero),
    m_rotation(Quaternion::Identity),
    m_eulerDegrees(Vector3::Zero),
    m_scale(Vector3(1.0f, 1.0f, 1.0f))
{
}

const Matrix& Transform::getGlobalMatrix() const
{
    if (m_dirty)
    {
        calculateMatrix();
        m_dirty = false;
    }
    return m_globalMatrix;
}

Matrix Transform::getNormalMatrix() const
{
    Matrix model = getGlobalMatrix();

    Matrix normal = model;
    normal.Translation(Vector3::Zero);

    normal = normal.Invert();
    normal = normal.Transpose();

    return normal;
}

void Transform::setRotation(const Quaternion& q)
{
    m_rotation = q;
    m_rotation.Normalize();

    Vector3 eulerRad = convertQuaternionToEulerAngles(m_rotation);
    m_eulerDegrees = eulerRad * (180.0f / XM_PI);

    markDirty();
}

void Transform::setRotationEuler(const Vector3& eulerDegrees)
{
    m_eulerDegrees = eulerDegrees;

    m_eulerDegrees.x = std::fmod(m_eulerDegrees.x, 360.0f);
    m_eulerDegrees.y = std::fmod(m_eulerDegrees.y, 360.0f);
    m_eulerDegrees.z = std::fmod(m_eulerDegrees.z, 360.0f);

    m_rotation = Quaternion::CreateFromYawPitchRoll(
        XMConvertToRadians(m_eulerDegrees.y),
        XMConvertToRadians(m_eulerDegrees.x),
        XMConvertToRadians(m_eulerDegrees.z)
    );

    m_rotation.Normalize();
    markDirty();
}


void Transform::markDirty()
{
    m_dirty = true;
    for (auto child : m_children)
    {
        child->GetTransform()->markDirty();
    }
}

void Transform::calculateMatrix() const
{
    Matrix local =
        Matrix::CreateScale(m_scale) *
        Matrix::CreateFromQuaternion(m_rotation) *
        Matrix::CreateTranslation(m_position);

    if (m_root)
        m_globalMatrix = local * m_root->getGlobalMatrix();
    else
        m_globalMatrix = local;
}


Vector3 Transform::convertQuaternionToEulerAngles(const Quaternion& q)
{
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    float roll = std::atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    float pitch;
    if (std::abs(sinp) >= 1.0f)
    {
        pitch = std::copysign(XM_PIDIV2, sinp);
    }
    else
    {
        pitch = std::asin(sinp);
    }

    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    float yaw = std::atan2(siny_cosp, cosy_cosp);

    return Vector3(pitch, yaw, roll);
}

void Transform::removeChild(int id)
{
    for (auto it = m_children.begin(); it != m_children.end(); ++it)
    {
        if ((*it)->GetID() == id)
        {
            m_children.erase(it);
            return;
        }
    }
}

bool Transform::isDescendantOf(const Transform* potentialParent) const
{
    const Transform* current = m_root;
    while (current)
    {
        if (current == potentialParent)
        {
            return true;
        }
        current = current->getRoot();
    }
    return false;
}

void Transform::drawUi()
{
    if (!ImGui::CollapsingHeader("Transform"))
    {
        return;
    }

    if (ImGui::DragFloat3("Position", &m_position.x, 0.01f))
    {
        markDirty();
    }

    if (ImGui::DragFloat3("Rotation", &m_eulerDegrees.x, 0.1f))
    {
        setRotationEuler(m_eulerDegrees);
    }

    if (ImGui::DragFloat3("Scale", &m_scale.x, 0.01f))
    {
        markDirty();
    }

    if (ImGui::Button("Reset"))
    {
        m_position = Vector3::Zero;
        m_scale = Vector3(1, 1, 1);
        setRotationEuler(Vector3::Zero);
    }
}
