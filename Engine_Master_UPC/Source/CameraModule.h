#pragma once
#include "Module.h"
#include "InputModule.h"

class CameraModule;

struct CameraCommand {

	enum Type {
		MOVE_FORWARD,
		MOVE_BACKWARD,
		MOVE_LEFT,
		MOVE_RIGHT,
		MOVE_UP,
		MOVE_DOWN,
		LOOK,
		FOCUS,
		ZOOM,
		ORBIT,

		COUNT
	};

	using Condition = std::function<bool()>;
	using Action = std::function<void(CameraModule*, float)>;

	Condition condition;
	Action action;
	Type type;

	CameraCommand(const Type type, Condition cond, Action act): type(type), condition(cond), action(act) {}

	bool Execute(CameraModule* camera, float deltaTime) const
	{
		if (condition && condition())
		{
			action(camera, deltaTime);
			return true;
		}
		return false;
	}
};

class CameraModule: public Module
{
public:
	CameraModule() = default;
	~CameraModule();
	bool init();
	bool postInit();
	void update();

	void UpdateAxes(const Vector3& pos, const Vector3& target);

	void SetFOV(const float fov, const float width, const float height);
	void SetAspectRatio(const float width, const float height);
	void SetPlaneDistances(const float near, const float far);
	void SetPosition(const Vector3& position);
	void SetOrientation(const Vector3& orientation);
	void LookAt(const Vector3& lookAt);
	void Resize(const float width, const float height);

	void CalculateProjectionMatrix();
	void CalculateViewMatrix();

	constexpr Matrix& GetProjectionMatrix() { return _proj; }
	constexpr Matrix& GetViewMatrix() { return _view; }
	constexpr Vector3& GetPosition() { return _eye; }

	constexpr float GetSpeed() { return _speed; }
	constexpr float GetSensitivity() { return _sensitivity; }
	constexpr Vector3& GetRight() { return _right; }
	constexpr Vector3& GetUp() { return _up; }
	constexpr Vector3& GetTarget() { return _target; }
	constexpr Vector3& GetForward() { return _forward; }


	//Camera Manipulation
	void Zoom(float amount);
	void Move(const Vector3& translation);
	void Focus(const Vector3& position, const Vector3& target);
	void Rotate(const Quaternion& rotation);
	void Orbit(const Quaternion& rotation, const Vector3& pivot);

private:

	Matrix _view = Matrix::Identity;
	Matrix _proj = Matrix::Identity;

	Vector3 _eye = Vector3(0.0f, 10.f,10.f);
	Vector3 _target = Vector3::Zero;
	Vector3 _up = Vector3::Up;

	Vector3 _forward = Vector3::Forward;
	Vector3 _right = Vector3::Right;

	Quaternion _rotation = Quaternion::Identity;
	float _sensitivity = 0.002f;

	bool _isDirty = false;

	float _fovH = XM_PIDIV4;
	float _fovV = XM_PIDIV4;
	float _aspectRatio = 0;
	float _nearPlane = 1.0f;
	float _farPlane = 1000.f;

	Vector2* _size;
	float _speed = 3.0f;
};

