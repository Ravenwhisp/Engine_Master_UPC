#include "Globals.h"
#include "CameraComponent.h"
#include "GameObject.h"
#include "Application.h"

extern Application* app;

CameraComponent::CameraComponent(int id, GameObject* gameObject) : Component(id, ComponentType::CAMERA, gameObject)
{
	Transform* t = m_owner->GetTransform();
	Vector3 position = t->getPosition();
	Quaternion rotation = t->getRotation();
	m_world = Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);
	recalculateFrustum();
	m_view = Matrix::CreateLookAt(position, position + t->getForward(), t->getUp());
	m_projection = Matrix::CreatePerspectiveFieldOfView(m_horizontalFov * (IM_PI / 180.0f), m_aspectRatio, m_nearPlane, m_farPlane);
}

void CameraComponent::recalculateFrustum() 
{
	Transform* t = m_owner->GetTransform();
	Vector3 position = t->getPosition();
	Quaternion rotation = t->getRotation();
	m_world = Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);

	m_frustum.calculateFrustumVerticesFromFrustum(m_world, m_horizontalFov, m_nearPlane, m_farPlane, m_aspectRatio, m_frustum.m_points);

	m_frustum.m_frontFace   = Plane(m_frustum.m_points[0], m_frustum.m_points[1], m_frustum.m_points[2]);
	m_frustum.m_backFace    = Plane(m_frustum.m_points[5], m_frustum.m_points[4], m_frustum.m_points[7]);
	m_frustum.m_topFace     = Plane(m_frustum.m_points[0], m_frustum.m_points[4], m_frustum.m_points[5]);
	m_frustum.m_bottomFace  = Plane(m_frustum.m_points[3], m_frustum.m_points[2], m_frustum.m_points[6]);
	m_frustum.m_leftFace    = Plane(m_frustum.m_points[4], m_frustum.m_points[0], m_frustum.m_points[3]);
	m_frustum.m_rightFace   = Plane(m_frustum.m_points[1], m_frustum.m_points[5], m_frustum.m_points[6]);
}

void CameraComponent::render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix)
{
	// For now just render the frustum itself. Later on, render the whole scene if we're in Game mode
	// Don't render it if this is the active camera.
	if (app->getCurrentCameraPerspective() != this)
	{
		m_frustum.render(m_world);
	}
}

void CameraComponent::update()
{
	
}

void CameraComponent::onTransformChange()
{
	Transform* t = m_owner->GetTransform();
	Vector3 position = t->getPosition();
	Quaternion rotation = t->getRotation();
	m_world = Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);
	recalculateFrustum();
	m_view = Matrix::CreateLookAt(position, position + t->getForward(), t->getUp());
	m_projection = Matrix::CreatePerspectiveFieldOfView(m_horizontalFov * (IM_PI / 180.0f), m_aspectRatio, m_nearPlane, m_farPlane);
}

void CameraComponent::drawUi() 
{
    ImGui::Separator();

	float fov = m_horizontalFov;
	float nearPlane = m_nearPlane;
	float farPlane = m_farPlane;
	float aspectRatio = m_aspectRatio;
    ImGui::DragFloat("FOV (horizontal)", &fov, 1.0f, 5.0f, 120.0f);
	ImGui::DragFloat("Near plane", &nearPlane, 0.005f, 0.01f, 1.0f);
	ImGui::DragFloat("Far plane", &farPlane, 1.0f, 10.0f, 100.0f);
	ImGui::DragFloat("Aspect ratio", &aspectRatio, 0.001f, 1.333333f, 2.333333f); // 4:3 to 21:9 -- FIXME : change to a dropdown menu with several options
	if (fov != m_horizontalFov || nearPlane != m_nearPlane || farPlane != m_farPlane || aspectRatio != m_aspectRatio) {
		m_horizontalFov = fov;
		m_nearPlane = nearPlane;
		m_farPlane = farPlane;
		m_aspectRatio = aspectRatio;
		recalculateFrustum();
		Transform* t = m_owner->GetTransform();
		Vector3 position = t->getPosition();
		m_view = Matrix::CreateLookAt(position, position + t->getForward(), t->getUp());
		m_projection = Matrix::CreatePerspectiveFieldOfView(m_horizontalFov * (IM_PI / 180.0f), m_aspectRatio, m_nearPlane, m_farPlane);
	}

	bool isThisCurrentActiveCamera = app->getActiveCamera() == this;
	ImGui::Checkbox("Set this camera as the active game camera", &isThisCurrentActiveCamera);
	if (isThisCurrentActiveCamera && app->getActiveCamera() != this)
	{
		app->setActiveCamera(this);
	}
	else if (!isThisCurrentActiveCamera && app->getActiveCamera() == this)
	{
		app->setActiveCamera(nullptr);
	}

	bool showThisCameraPerspective = app->getCurrentCameraPerspective() == this;
	ImGui::Checkbox("Show this camera's perspective", &showThisCameraPerspective);
	if (showThisCameraPerspective && app->getCurrentCameraPerspective() != this)
	{
		app->setCurrentCameraPerspective(this);
	}
	else if (!showThisCameraPerspective && app->getCurrentCameraPerspective() == this)
	{
		app->setCurrentCameraPerspective(nullptr);
	}
}

bool CameraComponent::cleanUp() {
	if (app->getCurrentCameraPerspective() == this)
	{
		app->setCurrentCameraPerspective(nullptr);
	}
	return true;
}