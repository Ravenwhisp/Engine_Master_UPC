#include "Globals.h"
#include "CameraComponent.h"

#include "Application.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

CameraComponent::CameraComponent(UID id, GameObject* gameObject) : Component(id, ComponentType::CAMERA, gameObject)
{
	Transform* t = m_owner->GetTransform();
	Vector3 position = t->getPosition();
	Quaternion rotation = t->getRotation();
	m_world = Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);
	recalculateFrustum();
	m_view = Matrix::CreateLookAt(position, position + t->getForward(), t->getUp());
	m_projection = Matrix::CreatePerspectiveFieldOfView(m_horizontalFov * (IM_PI / 180.0f) / m_aspectRatio, m_aspectRatio, m_nearPlane, m_farPlane);
}

std::unique_ptr<Component> CameraComponent::clone(GameObject* newOwner) const
{
	std::unique_ptr<CameraComponent> clonedComponent = std::make_unique<CameraComponent>(m_uuid, newOwner);

	clonedComponent->m_horizontalFov = this->m_horizontalFov;
	clonedComponent->m_nearPlane = this->m_nearPlane;
	clonedComponent->m_farPlane = this->m_farPlane;
	clonedComponent->m_aspectRatio = this->m_aspectRatio;

	// No need to clone the matrices nor the frustum, as they will be recalculated in the onTransformChange() method when the component is added to the new owner
	return clonedComponent;
}

void CameraComponent::recalculateFrustum() 
{
	Transform* t = m_owner->GetTransform();
	Vector3 position = t->getPosition();
	Quaternion rotation = t->getRotation();
	m_world = Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);

	m_frustum.calculateFrustumVerticesFromFrustum(m_world, m_horizontalFov, m_nearPlane, m_farPlane, m_aspectRatio, m_frustum.m_points);

	Matrix vp = m_view * m_projection;

	m_frustum.m_leftFace = Plane(vp._14 + vp._11, vp._24 + vp._21, vp._34 + vp._31, vp._44 + vp._41);
	m_frustum.m_leftFace.Normalize();

	m_frustum.m_rightFace = Plane(vp._14 - vp._11, vp._24 - vp._21, vp._34 - vp._31, vp._44 - vp._41);
	m_frustum.m_rightFace.Normalize();

	m_frustum.m_bottomFace = Plane(vp._14 + vp._12, vp._24 + vp._22, vp._34 + vp._32, vp._44 + vp._42);
	m_frustum.m_bottomFace.Normalize();

	m_frustum.m_topFace = Plane(vp._14 - vp._12, vp._24 - vp._22, vp._34 - vp._32, vp._44 - vp._42);
	m_frustum.m_topFace.Normalize();

	m_frustum.m_frontFace = Plane(vp._13, vp._23, vp._33, vp._43);
	m_frustum.m_frontFace.Normalize();

	m_frustum.m_backFace = Plane(vp._14 - vp._13, vp._24 - vp._23, vp._34 - vp._33, vp._44 - vp._43);
	m_frustum.m_backFace.Normalize();

}

void CameraComponent::update()
{
	// No se si es optimo, pero es para comprobar que aqui esta el error
	PERF_LOGIC("CameraComponent::update");
	onTransformChange();
}

void CameraComponent::onTransformChange()
{
	PERF_LOGIC("onTransformChange");
	Transform* t = m_owner->GetTransform();
	Vector3 position = t->getPosition();
	Quaternion rotation = t->getRotation();
	m_world = Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(position);
	{
		PERF_LOGIC("recalculatefrustum");
		recalculateFrustum();
	}
	m_view = Matrix::CreateLookAt(position, position + t->getForward(), t->getUp());

	{
		PERF_LOGIC("rebuildMRcache");
		app->getModuleScene()->rebuildMeshRenderersCache();
	}
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

	if (fov != m_horizontalFov || nearPlane != m_nearPlane || farPlane != m_farPlane || aspectRatio != m_aspectRatio) 
	{
		m_horizontalFov = fov;
		m_nearPlane = nearPlane;
		m_farPlane = farPlane;
		m_aspectRatio = aspectRatio;
		recalculateFrustum();
		Transform* t = m_owner->GetTransform();
		Vector3 position = t->getPosition();
		m_view = Matrix::CreateLookAt(position, position + t->getForward(), t->getUp());
		m_projection = Matrix::CreatePerspectiveFieldOfView(m_horizontalFov * (IM_PI / 180.0f) / m_aspectRatio, m_aspectRatio, m_nearPlane, m_farPlane);
	}
	
	if (ImGui::Button("Set as Default Camera"))
	{
		app->getModuleScene()->getScene()->setDefaultCamera(this);
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

void CameraComponent::debugDraw()
{
	m_frustum.render(m_world);
}

bool CameraComponent::cleanUp() 
{
	if (app->getCurrentCameraPerspective() == this)
	{
		app->setCurrentCameraPerspective(nullptr);
	}
	if (app->getModuleScene()->getScene()->getDefaultCamera() == this)
	{
		app->getModuleScene()->getScene()->setDefaultCamera(nullptr);
	}
	return true;
}

rapidjson::Value CameraComponent::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value componentInfo(rapidjson::kObjectType);

	componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
	componentInfo.AddMember("ComponentType", unsigned int(ComponentType::CAMERA), domTree.GetAllocator());
	componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

	componentInfo.AddMember("HorizontalFOV", m_horizontalFov, domTree.GetAllocator());
	componentInfo.AddMember("NearPlane", m_nearPlane, domTree.GetAllocator());
	componentInfo.AddMember("FarPlane", m_farPlane, domTree.GetAllocator());
	componentInfo.AddMember("AspectRatio", m_aspectRatio, domTree.GetAllocator());

	return componentInfo;
}

bool CameraComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
	if (componentInfo.HasMember("HorizontalFOV")) 
	{
		m_horizontalFov = componentInfo["HorizontalFOV"].GetFloat();
	}

	if (componentInfo.HasMember("NearPlane")) 
	{
		m_nearPlane = componentInfo["NearPlane"].GetFloat();
	}

	if (componentInfo.HasMember("FarPlane")) 
	{
		m_farPlane = componentInfo["FarPlane"].GetFloat();
	}

	if (componentInfo.HasMember("AspectRatio")) 
	{
		m_aspectRatio = componentInfo["AspectRatio"].GetFloat();
	}

	recalculateFrustum();

	return true;
}
