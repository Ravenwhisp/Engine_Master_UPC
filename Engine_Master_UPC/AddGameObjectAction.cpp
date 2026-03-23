#include "Globals.h"
#include "AddGameObjectAction.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include "Scene.h"
#include <HierarchyUtils.h>

AddGameObjectAction::AddGameObjectAction(Scene* scene, GameObject* parent): m_scene(scene), m_parentID(parent ? parent->GetID() : 0)
{
}

GameObject* AddGameObjectAction::run()
{
    if (!m_scene) return nullptr;

    GameObject* created = m_scene->createGameObject();

    if (m_parentID != 0)
    {
        GameObject* parent = HierarchyUtils::findByUID(m_scene, m_parentID);
        HierarchyUtils::reparent(m_scene, created, parent);
    }

    app->getModuleEditor()->setSelectedGameObject(created);
    return created;
}
