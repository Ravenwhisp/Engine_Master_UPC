#include "pch.h"
#include "DebugGetObjectsArea.h"

IMPLEMENT_SCRIPT_FIELDS(DebugGetObjectsArea, 
    SERIALIZED_FLOAT(m_radius, "Circle Radius", 0.0f, 10.0f, 0.1f),
    )

DebugGetObjectsArea::DebugGetObjectsArea(GameObject* owner)
    : Script(owner)
{
}

void DebugGetObjectsArea::Start()
{

}

void DebugGetObjectsArea::Update()
{
	const Vector3 center = GameObjectAPI::getTransform(getOwner())->getPosition();
	std::vector<GameObject*> objectsInArea = SceneAPI::getObjectsInCircularArea(Vector2(center.x, center.z), m_radius);

	//con estos objetos, haremos que miren hacia el centro del area, para demostrar que estan dentro
	for (GameObject* object : objectsInArea)
	{
		Transform* transform = object->GetTransform();
		if (transform)
		{
			Vector3 dir = center - transform->getPosition();
			float angleRadians = atan2f(dir.x, dir.z);
			float angleDegrees = angleRadians * (180.0f / 3.14159265f);
			TransformAPI::setGlobalRotationEuler(transform, Vector3(0.0f, angleDegrees, 0.0f));
		}
	}
}

void DebugGetObjectsArea::drawGizmo()
{
    const Vector3 center = GameObjectAPI::getTransform(getOwner())->getPosition();
	//DebugDrawAPI::drawCircle(center, Vector3::UnitY, Vector3(0.0f, 1.0f, 1.0f), m_radius);
	DebugDrawAPI::drawSphere(center, Vector3(0.0f, 1.0f, 1.0f), m_radius);
}

IMPLEMENT_SCRIPT(DebugGetObjectsArea)