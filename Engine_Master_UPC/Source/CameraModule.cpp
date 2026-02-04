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
	CalculateViewMatrix();
	return true;
}

bool CameraModule::postInit()
{
	return true;
}

void CameraModule::UpdateAxes(const Vector3& pos, const Vector3& target)
{
	_forward = target - pos;
	_forward.Normalize();

	_right = _forward.Cross(_up);
	_right.Normalize();
}

void CameraModule::Zoom(float amount)
{
	_eye += _forward * amount;
	_target += _forward * amount;
	_isDirty = true;
	UpdateAxes(_eye, _target);
}


void CameraModule::Move(const Vector3& translation)
{
	_eye += translation;
	_target += translation;
	_isDirty = true;
}


void CameraModule::Focus(const Vector3& position, const Vector3& target)
{
	_eye = position;
	_target = target;
	_isDirty = true;
	UpdateAxes(_eye, _target);
}

void CameraModule::Rotate(const Quaternion& rotation)
{
	Vector3 dir = _target - _eye;
	dir = Vector3::Transform(dir, Matrix::CreateFromQuaternion(rotation));
	_target = _eye + dir;
	_isDirty = true;
	UpdateAxes(_eye, _target);
}

void CameraModule::Orbit(const Quaternion& rotation, const Vector3& pivot)
{
	Vector3 offset = _eye - pivot;
	offset = Vector3::Transform(offset, Matrix::CreateFromQuaternion(rotation));
	_eye = pivot + offset;
	_isDirty = true;
	UpdateAxes(_eye, _target);
}


void CameraModule::update()
{
	if (_isDirty) {
		CalculateViewMatrix();
		CalculateProjectionMatrix();
		_isDirty = false;
	}
}

void CameraModule::SetFOV(const float fov, const float width, const float height)
{
	_fovH = fov;
	SetAspectRatio(width, height);

	_isDirty = true;
}

void CameraModule::SetAspectRatio(float width, float height)
{
	_aspectRatio = (float)width / (float)height;

	// Recompute vertical FOV based on the new aspect ratio
	_fovV = 2.0f * atan(tan(_fovH * 0.5f) / _aspectRatio);

	_isDirty = true;
}

void CameraModule::SetPlaneDistances(const float nearPlane, const float farPlane)
{
	_nearPlane = nearPlane;
	_farPlane = farPlane;
	_isDirty = true;
}

void CameraModule::SetPosition(const Vector3& position)
{
	_eye = position;
	_isDirty = true;
}

void CameraModule::SetOrientation(const Vector3& orientation)
{
	_target = orientation;
	_isDirty = true;
}

void CameraModule::LookAt(const Vector3& lookAt)
{
	_eye = lookAt;
	_isDirty = true;
}

void CameraModule::Resize(float width, float height)
{
	SetAspectRatio(width, height);
}

void CameraModule::CalculateProjectionMatrix()
{
	_proj = Matrix::CreatePerspectiveFieldOfView(_fovV, _aspectRatio, _nearPlane, _farPlane);
}

void CameraModule::CalculateViewMatrix() {
		
	_view = Matrix::CreateLookAt(_eye, _target, _up);
}
