#include "Globals.h"
#include "CommandInstantiatePrefab.h"

#include "Application.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include <PrefabManager.h>
#include <HierarchyUtils.h>

CommandInstantiatePrefab::CommandInstantiatePrefab(Scene* scene,
    const std::filesystem::path& sourcePath,
    GameObject* parent)
    : m_scene(scene)
    , m_source(sourcePath)
    , m_parentID(parent ? parent->GetID() : 0)
{
}

void CommandInstantiatePrefab::run()
{
    if (!m_scene) return;

    m_result = PrefabManager::instantiatePrefab(m_source, m_scene);
    if (!m_result) return;

    if (m_parentID != 0)
    {
        GameObject* parent = HierarchyUtils::findByUID(m_scene, m_parentID);
        HierarchyUtils::reparent(m_scene, m_result, parent);
    }

    app->getModuleEditor()->setSelectedGameObject(m_result);
}

GameObject* CommandInstantiatePrefab::getResult() const
{
    return m_result;
}
