#include "Globals.h"
#include "Transform.h"
#include "GameObject.h"

Transform::Transform(int id, GameObject* gameObject) :
    Component(id, ComponentType::TRANSFORM, gameObject),
    m_dirty(false),
    m_root(nullptr),
    m_globalMatrix(Matrix::Identity),
    m_position(Vector3::Zero),
    m_rotation(Quaternion::Identity),
    m_scale(Vector3(1.0f, 1.0f, 1.0f))
{
};

const Matrix& Transform::getGlobalMatrix() const
{
    if (m_dirty) {
        calculateMatrix();
        m_dirty = false;
    }
    return m_globalMatrix;
}

void Transform::markDirty()
{
    m_dirty = true;
    for (auto child : m_children) child->GetTransform()->markDirty();
}


Vector3 Transform::convertQuaternionToEulerAngles(const Quaternion &rotation)
{
    Quaternion quaternion = rotation;

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

void Transform::calculateMatrix() const
{
    if (m_root != nullptr)
    {
        m_globalMatrix = Matrix::CreateScale(m_scale) * Matrix::CreateFromQuaternion(m_rotation) * Matrix::CreateTranslation(m_position) * (m_root->getGlobalMatrix());
    }
    else
    {
        m_globalMatrix = Matrix::CreateScale(m_scale) * Matrix::CreateFromQuaternion(m_rotation) * Matrix::CreateTranslation(m_position);
    }
}

void Transform::removeChild(int id)
{
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->GetID() == id) {
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
            return true;
        current = current->getRoot();
    }
    return false;
}

void Transform::drawUi() {
    if (ImGui::CollapsingHeader("Transform")) {

        if (ImGui::DragFloat3("Position", &m_position.x, 0.01f))
        {
            m_dirty = true;
        }

        Vector3 rot = convertQuaternionToEulerAngles(m_rotation);
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
