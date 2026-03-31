#include "Globals.h"
#include "CommandRenameGameObject.h"
#include <HierarchyUtils.h>
#include "GameObject.h"

CommandRenameGameObject::CommandRenameGameObject(Scene* scene, GameObject* target, const std::string& newName) 
    : m_scene(scene), m_targetID(target ? target->GetID() : 0), m_newName(newName)
{
}

void CommandRenameGameObject::run()
{
    if (!m_scene || m_targetID == 0 || m_newName.empty()) return;

    if (GameObject* go = HierarchyUtils::findByUID(m_scene, m_targetID))
    {
        go->SetName(m_newName);
    }
}
