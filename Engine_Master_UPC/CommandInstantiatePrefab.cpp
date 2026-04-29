#include "Globals.h"
#include "CommandInstantiatePrefab.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleAssets.h"

#include "GameObject.h"
#include <HierarchyUtils.h>
#include "PrefabAsset.h"

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
    UID prefabId = app->getModuleAssets()->findUID(m_source);
    auto prefab = app->getModuleAssets()->load<PrefabAsset>(makeRef(prefabId));
    m_result = prefab->spawnPrefab().get();

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
