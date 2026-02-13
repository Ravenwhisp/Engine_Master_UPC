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

	void updateAxes(const Vector3& pos, const Vector3& target);

	void setFOV(const float fov, const float width, const float height);
	void setAspectRatio(const float width, const float height);
	void setPlaneDistances(const float near, const float far);
	void setPosition(const Vector3& position);
	void setOrientation(const Vector3& orientation);
	void lookAt(const Vector3& lookAt);
	void resize(const float width, const float height);

	void calculateProjectionMatrix();
	void calculateViewMatrix();

	constexpr Matrix&	getProjectionMatrix() { return m_proj; }
	constexpr Matrix&	getViewMatrix() { return m_view; }
	constexpr Vector3&	getPosition() { return m_eye; }

	constexpr float		getSpeed() { return m_speed; }
	constexpr float		getSensitivity() { return m_sensitivity; }
	constexpr Vector3&	getRight() { return m_right; }
	constexpr Vector3&	getUp() { return m_up; }
	constexpr Vector3&	getTarget() { return m_target; }
	constexpr Vector3&	getForward() { return m_forward; }


	//Camera Manipulation
	void zoom(float amount);
	void move(const Vector3& translation);
	void focus(const Vector3& position, const Vector3& target);
	void rotate(const Quaternion& rotation);
	void orbit(const Quaternion& rotation, const Vector3& pivot);

private:

	Matrix	m_view = Matrix::Identity;
	Matrix	m_proj = Matrix::Identity;

	Vector3 m_eye = Vector3(0.0f, 10.f,10.f);
	Vector3 m_target = Vector3::Zero;
	Vector3 m_up = Vector3::Up;

	Vector3 m_forward = Vector3::Forward;
	Vector3 m_right = Vector3::Right;

	Quaternion m_rotation = Quaternion::Identity;
	float m_sensitivity = 0.002f;

	bool m_isDirty = false;

	float m_fovH = XM_PIDIV4;
	float m_fovV = XM_PIDIV4;
	float m_aspectRatio = 0;
	float m_nearPlane = 1.0f;
	float m_farPlane = 1000.f;

	Vector2* m_size;
	float m_speed = 3.0f;
};

