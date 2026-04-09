#include "Globals.h"
#include "CommandCreateGameObject.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include "Scene.h"
#include "ComponentType.h"
#include "UID.h"
#include "LightComponent.h"

#include <HierarchyUtils.h>

CommandCreateGameObject::CommandCreateGameObject(Scene* scene, GameObject* parent, ComponentType componentType)
    : m_scene(scene), m_parentID(parent ? parent->GetID() : 0), m_componentType(componentType)
{
}

void CommandCreateGameObject::run()
{
    if (!m_scene) return;

    m_result = m_scene->createGameObject();

    if (m_parentID != 0)
    {
        GameObject* parent = HierarchyUtils::findByUID(m_scene, m_parentID);
        HierarchyUtils::reparent(m_scene, m_result, parent);
    }

    switch (m_componentType)
    {
    case ComponentType::CAMERA:
        m_result->SetName("New Camera");
        m_result->AddComponent(ComponentType::CAMERA);
        break;
    case ComponentType::LIGHT:
        m_result->SetName("New Point Light");
        m_result->AddComponentWithUID(ComponentType::LIGHT, GenerateUID());
        break;
    case ComponentType::MODEL:
        m_result->SetName("New Model");
        m_result->AddComponent(ComponentType::MODEL);
        break;
    case ComponentType::CANVAS:
        m_result->SetName("New Canvas");
        m_result->AddComponentWithUID(ComponentType::CANVAS, GenerateUID());
        break;
    case ComponentType::UIBUTTON:
        m_result->SetName("New Button");
        m_result->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
        m_result->AddComponentWithUID(ComponentType::UIBUTTON, GenerateUID());
        m_result->AddComponentWithUID(ComponentType::UIIMAGE, GenerateUID());
        break;
    case ComponentType::UITEXT:
        m_result->SetName("New Text");
        m_result->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
        m_result->AddComponentWithUID(ComponentType::UITEXT, GenerateUID());
        break;
    case ComponentType::UIIMAGE:
        m_result->SetName("New Image");
        m_result->AddComponentWithUID(ComponentType::TRANSFORM2D, GenerateUID());
        m_result->AddComponentWithUID(ComponentType::UIIMAGE, GenerateUID());
        break;
    default:
        break;
    }

    app->getModuleEditor()->setSelectedGameObject(m_result);
}

GameObject* CommandCreateGameObject::getResult() const
{
    return m_result;
}
