#pragma once
#include "Globals.h"
#include "Component.h"

/// <summary>
/// Represents a 3D transformation including position, rotation, and scale.
/// </summary>
class Transform: public Component
{
public:
	Transform() = default;
	~Transform() = default;

	const Vector3&		getPosition() const { return m_position; }
	const Quaternion&	getRotation() const { return m_rotation; }
	const Vector3&		getScale() const { return m_scale; }

	void setPosition(const Vector3& pos) { m_position = pos; }
	void setRotation(const Quaternion& rot) { m_rotation = rot; }
	void setScale(const Vector3& scl) { m_scale = scl; }

	Vector3 getForward() const;
	Matrix& getWorldMatrix();
	Matrix& getNormalMatrix();

	void	setWorldMatrix(Matrix& matrix) { matrix.Decompose(m_scale, m_rotation, m_position); }
private:
	Vector3 m_position = Vector3::Zero;
	Quaternion m_rotation = Quaternion::Identity;
	Vector3 m_scale = Vector3::One;

	Matrix m_worldMatrix = Matrix::Identity;
};

