#include "Globals.h"
#include "CameraModule.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "TimeModule.h"
#include "Application.h"
#include "D3D12Module.h"
#include "EditorModule.h"
#include "SceneEditor.h"


CameraModule::~CameraModule()
{

}

bool CameraModule::init()
{
	calculateViewMatrix();
	return true;
}

bool CameraModule::postInit()
{
	return true;
}

void CameraModule::updateAxes(const Vector3& pos, const Vector3& target)
{
	m_forward = target - pos;
	m_forward.Normalize();

	m_right = m_forward.Cross(m_up);
	m_right.Normalize();
}

void CameraModule::zoom(float amount)
{
	m_eye += m_forward * amount;
	m_target += m_forward * amount;
	m_isDirty = true;
	updateAxes(m_eye, m_target);
}


void CameraModule::move(const Vector3& translation)
{
	m_eye += translation;
	m_target += translation;
	m_isDirty = true;
}


void CameraModule::focus(const Vector3& position, const Vector3& target)
{
	m_eye = position;
	m_target = target;
	m_isDirty = true;
	updateAxes(m_eye, m_target);
}

void CameraModule::rotate(const Quaternion& rotation)
{
	Vector3 dir = m_target - m_eye;
	dir = Vector3::Transform(dir, Matrix::CreateFromQuaternion(rotation));
	m_target = m_eye + dir;
	m_isDirty = true;
	updateAxes(m_eye, m_target);
}

void CameraModule::orbit(const Quaternion& rotation, const Vector3& pivot)
{
	Vector3 offset = m_eye - pivot;
	offset = Vector3::Transform(offset, Matrix::CreateFromQuaternion(rotation));
	m_eye = pivot + offset;
	m_isDirty = true;
	updateAxes(m_eye, m_target);
}


void CameraModule::update()
{
	if (m_isDirty) 
	{
		calculateViewMatrix();
		calculateProjectionMatrix();
		m_isDirty = false;
	}
}

void CameraModule::setFOV(const float fov, const float width, const float height)
{
	m_fovH = fov;
	setAspectRatio(width, height);

	m_isDirty = true;
}

void CameraModule::setAspectRatio(float width, float height)
{
	m_aspectRatio = (float)width / (float)height;

	// Recompute vertical FOV based on the new aspect ratio
	m_fovV = 2.0f * atan(tan(m_fovH * 0.5f) / m_aspectRatio);

	m_isDirty = true;
}

void CameraModule::setPlaneDistances(const float nearPlane, const float farPlane)
{
	m_nearPlane = nearPlane;
	m_farPlane = farPlane;
	m_isDirty = true;
}

void CameraModule::setPosition(const Vector3& position)
{
	m_eye = position;
	m_isDirty = true;
}

void CameraModule::setOrientation(const Vector3& orientation)
{
	m_target = orientation;
	m_isDirty = true;
}

void CameraModule::lookAt(const Vector3& lookAt)
{
	m_eye = lookAt;
	m_isDirty = true;
}

void CameraModule::resize(float width, float height)
{
	setAspectRatio(width, height);
}

void CameraModule::calculateProjectionMatrix()
{
	m_proj = Matrix::CreatePerspectiveFieldOfView(m_fovV, m_aspectRatio, m_nearPlane, m_farPlane);
}

void CameraModule::calculateViewMatrix() {
		
	m_view = Matrix::CreateLookAt(m_eye, m_target, m_up);
}
