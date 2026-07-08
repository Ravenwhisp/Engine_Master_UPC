#pragma once
#include "Globals.h"
#include "Component.h"
#include "Frustum.h"
#include "IDebugDrawable.h"

class CameraComponent : public Component
{
public:
	CameraComponent(UID id, GameObject* gameObject);
	virtual std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void lateUpdate() override;

	float getFov() const { return m_horizontalFov; }
	void setFov(float fov) { m_horizontalFov = fov; updateCameraMatrices(); }

	float getNearPlane() const { return m_nearPlane; }
	void setNearPlane(float nearPlane) { m_nearPlane = nearPlane; updateCameraMatrices(); }

	float getFarPlane() const { return m_farPlane; }
	void setFarPlane(float farPlane) { m_farPlane = farPlane; updateCameraMatrices(); }

	const float getAspectRatio() const { return m_aspectRatio; }
	void setAspectRatio(const float aspectRatio) { m_aspectRatio = aspectRatio; }

	const Matrix& getWorldMatrix() const { return m_world; }

	const Matrix& getViewMatrix() const { return m_view; }

	const Matrix& getProjectionMatrix() const { return m_projection; }

	const Engine::Frustum& getFrustum() const { return m_frustum; }

	void recalculateFrustum();

	void drawUi() override;
	void debugDraw() override;

	void onTransformChange() override;

	bool cleanUp() override;

	void serialize(IArchive& archive) override;

private:
	void updateCameraMatrices();

private:
	float m_horizontalFov = 90.0f;
	float m_nearPlane = 0.5f;
	float m_farPlane = 25.0f;
	float m_aspectRatio = 16.0f / 9.0f;

	Matrix m_world = {};
	Matrix m_view = {};
	Matrix m_projection = {};

	Engine::Frustum m_frustum = {};
};