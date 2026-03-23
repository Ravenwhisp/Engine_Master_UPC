#include "Globals.h"
#include "RemoveGameObjectAction.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "Scene.h"
#include "GameObject.h"
#include <HierarchyUtils.h>


RemoveGameObjectAction::RemoveGameObjectAction(Scene* scene, GameObject* target)
    : m_scene(scene)
    , m_targetID(target ? target->GetID() : 0)
{
}

void RemoveGameObjectAction::run()
{
    if (!m_scene || m_targetID == 0) return;

    if (app->getModuleEditor()->getSelectedGameObject() == HierarchyUtils::findByUID(m_scene, m_targetID))
    {
        app->getModuleEditor()->setSelectedGameObject(nullptr);
    }

    m_scene->removeGameObject(m_targetID);
}
