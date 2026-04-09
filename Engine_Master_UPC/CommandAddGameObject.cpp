#include "Globals.h"
#include "CommandAddGameObject.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include "Scene.h"
#include <HierarchyUtils.h>

CommandAddGameObject::CommandAddGameObject(Scene* scene, GameObject* parent)
    : m_scene(scene), m_parentID(parent ? parent->GetID() : 0)
{
}

void CommandAddGameObject::run()
{
    if (!m_scene) return;

    m_result = m_scene->createGameObject();

    if (m_parentID != 0)
    {
        GameObject* parent = HierarchyUtils::findByUID(m_scene, m_parentID);
        HierarchyUtils::reparent(m_scene, m_result, parent);
    }

    app->getModuleEditor()->setSelectedGameObject(m_result);
}

GameObject* CommandAddGameObject::getResult() const
{
    return m_result;
}
