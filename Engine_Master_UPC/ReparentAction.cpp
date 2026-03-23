#include "Globals.h"
#include "ReparentAction.h"

#include <HierarchyUtils.h>
#include "GameObject.h"

ReparentAction::ReparentAction(Scene* scene, GameObject* child, GameObject* newParent)
    : m_scene(scene)
    , m_childID(child ? child->GetID() : 0)
    , m_newParentID(newParent ? newParent->GetID() : 0)
{
}

void ReparentAction::run()
{
    if (!m_scene || m_childID == 0) return;

    GameObject* child = HierarchyUtils::findByUID(m_scene, m_childID);
    GameObject* newParent = HierarchyUtils::findByUID(m_scene, m_newParentID);
    HierarchyUtils::reparent(m_scene, child, newParent);
}

