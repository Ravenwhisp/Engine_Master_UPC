#include "Globals.h"
#include "Transform.h"
#include "GameObject.h"
#include <cmath>

Transform::Transform(UID id, GameObject* gameObject) :
    Component(id, ComponentType::TRANSFORM, gameObject),
    m_dirty(true),
    m_root(nullptr),
    m_globalMatrix(Matrix::Identity),
    m_position(Vector3::Zero),
    m_rotation(Quaternion::Identity),
    m_eulerDegrees(Vector3::Zero),
    m_scale(Vector3(0.01f, 0.01f, 0.01f))
{
}

const Matrix& Transform::getGlobalMatrix() const
{
    if (m_dirty)
    {
        calculateMatrix();
        m_dirty = false;
        m_owner->onTransformChange();
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

void Transform::setFromGlobalMatrix(const Matrix& globalMatrix)
{
    Matrix localMatrix = globalMatrix;

    if (m_root)
    {
        Matrix parentInv = m_root->getGlobalMatrix().Invert();
        localMatrix = globalMatrix * parentInv;
    }

    m_globalMatrix = globalMatrix;

    Vector3 scale;
    Quaternion rotation;
    Vector3 position;

    if (localMatrix.Decompose(scale, rotation, position))
    {
        m_scale = scale;
        m_rotation = rotation;
        m_rotation.Normalize();
        m_position = position;

        Vector3 eulerRad = m_rotation.ToEuler();
        m_eulerDegrees = eulerRad * (180.0f / XM_PI);;
    }

    markDirty();
}

void Transform::setRotation(const Quaternion& q)
{
    m_rotation = q;
    m_rotation.Normalize();

    Vector3 eulerRad = m_rotation.ToEuler();
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

Vector3 Transform::getRight() const
{
    const Matrix& globalMatrix = getGlobalMatrix();

    Vector3 right(globalMatrix._11, globalMatrix._12, globalMatrix._13);
    right.Normalize();
    return right;
}

Vector3 Transform::getUp() const
{
    const Matrix& globalMatrix = getGlobalMatrix();

    Vector3 up(globalMatrix._21, globalMatrix._22, globalMatrix._23);   
    up.Normalize();
    return up;
}

Vector3 Transform::getForward() const
{
    const Matrix& globalMatrix = getGlobalMatrix();

    Vector3 forward(globalMatrix._31, globalMatrix._32, globalMatrix._33);
    forward.Normalize();
    return forward;
}

void Transform::calculateMatrix() const
{
    Matrix local =
        Matrix::CreateScale(m_scale) *
        Matrix::CreateFromQuaternion(m_rotation) *
        Matrix::CreateTranslation(m_position);

    if (m_root)
    {
        m_globalMatrix = local * m_root->getGlobalMatrix();
    }
    else
    {
        m_globalMatrix = local;
    }
}

void Transform::removeChild(UID id)
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
