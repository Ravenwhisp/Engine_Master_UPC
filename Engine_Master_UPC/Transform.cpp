#include "Globals.h"
#include "Transform.h"

Vector3 Transform::getForward() const
{
	return Vector3::Transform(Vector3::Forward, m_rotation);
}

Matrix& Transform::getWorldMatrix()
{
	m_worldMatrix = Matrix::CreateScale(m_scale) *
		Matrix::CreateFromQuaternion(m_rotation) *
		Matrix::CreateTranslation(m_position);
	return m_worldMatrix;
}

Matrix& Transform::getNormalMatrix()
{
	Matrix normal = getWorldMatrix();
	normal.Translation(Vector3::Zero);
	normal.Invert();
	normal.Transpose();

	return normal;
}
