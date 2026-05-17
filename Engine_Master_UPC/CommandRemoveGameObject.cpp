#include "Globals.h"
#include "CommandRemoveGameObject.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "Scene.h"
#include "GameObject.h"
#include <HierarchyUtils.h>


CommandRemoveGameObject::CommandRemoveGameObject(Scene* scene, GameObject* target)
    : m_scene(scene)
    , m_targetID(target ? target->GetID() : 0)
{
}

void CommandRemoveGameObject::run()
{
    if (!m_scene || m_targetID == 0) return;

    GameObject* go = app->getModuleEditor()->getSelectedGameObject();
    
    if (!go)
    {
        return;
    }

    if (go == HierarchyUtils::findByUID(m_scene, m_targetID))
    {
        app->getModuleEditor()->setSelectedGameObject(nullptr);
    }

    m_scene->removeGameObject(m_targetID);
}
