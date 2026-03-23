#include "Globals.h"
#include "InstantiatePrefabAction.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include <PrefabManager.h>
#include <HierarchyUtils.h>

InstantiatePrefabAction::InstantiatePrefabAction(Scene* scene,
    const std::filesystem::path& sourcePath,
    GameObject* parent)
    : m_scene(scene)
    , m_source(sourcePath)
    , m_parentID(parent ? parent->GetID() : 0)
{
}

GameObject* InstantiatePrefabAction::run()
{
    if (!m_scene) return nullptr;

    GameObject* spawned = PrefabManager::instantiatePrefab(m_source, m_scene);
    if (!spawned) return nullptr;

    if (m_parentID != 0)
    {
        GameObject* parent = HierarchyUtils::findByUID(m_scene, m_parentID);
        HierarchyUtils::reparent(m_scene, spawned, parent);
    }

    app->getModuleEditor()->setSelectedGameObject(spawned);
    return spawned;
}