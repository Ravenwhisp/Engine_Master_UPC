#include "pch.h"
#include "ProjectileController.h"

IMPLEMENT_SCRIPT_FIELDS(ProjectileController,
    SERIALIZED_FLOAT(m_minSpeed, "Min Speed", 0.0f, 150.0f, 0.1f),
    SERIALIZED_FLOAT(m_maxSpeed, "Max Speed", 0.0f, 150.0f, 0.1f),
    SERIALIZED_FLOAT(m_scale, "Scale", 0.0f, 1.0f, 0.1f),
    SERIALIZED_COMPONENT_REF(m_target, "Target", ComponentType::TRANSFORM),
    SERIALIZED_STRING(m_prefabToInstantiate, "Prefab path")
)

ProjectileController::ProjectileController(GameObject* owner)
    : Script(owner)
{
    m_objectTransform = GameObjectAPI::getTransform(owner);

}

void ProjectileController::Start()
{
    m_speed = m_minSpeed + m_scale * (m_maxSpeed - m_minSpeed);

    m_canPierce = (m_speed == m_maxSpeed);
}

void ProjectileController::Update()
{
    Vector3 translation = m_speed * Time::getDeltaTime() * TransformAPI::getForward(m_objectTransform);
    TransformAPI::translate(m_objectTransform, translation);

    if (hit())
    {
        if (m_canPierce) m_canPierce = false;
        //else GameObjectAPI::delete();
    }

    if (Input::isKeyDown(KeyCode::A)) {

        if (not pressed)
        {
            GameObject* test = GameObjectAPI::instantiatePrefab(m_prefabToInstantiate.c_str(), TransformAPI::getPosition(m_objectTransform), TransformAPI::getEulerDegrees(m_objectTransform), m_objectTransform->getOwner());
            if (test) Debug::log("I exist!");

            if (Input::isKeyDown(KeyCode::D)) GameObjectAPI::removeGameObject(m_objectTransform->getOwner());

            pressed = true;
        }
    }
    else pressed = false;
}

bool ProjectileController::hit()
{
    return false;
}

IMPLEMENT_SCRIPT(ProjectileController)