#include "Globals.h"
#include "CommandSaveGameObjectAsPrefab.h"


#include "Application.h"
#include "ModuleFileSystem.h"
#include "ModuleAssets.h"

#include "GameObject.h"
#include <PrefabManager.h>
#include "PrefabAsset.h"
#include <Extensions.h>

CommandSaveGameObjectAsPrefab::CommandSaveGameObjectAsPrefab(
    GameObject* go,
    const std::filesystem::path& targetDir)
    : m_go(go)
    , m_targetDir(targetDir)
{
}

void CommandSaveGameObjectAsPrefab::run()
{
    if (!m_go) return;

    std::filesystem::path savePath = m_targetDir / (m_go->GetName() + PREFAB_EXTENSION);
    int suffix = 1;
    while (app->getModuleFileSystem()->exists(savePath))
    {
        savePath = m_targetDir / (m_go->GetName() + "_" + std::to_string(suffix++) + PREFAB_EXTENSION);
    }

    if (!PrefabManager::createPrefab(m_go, savePath))
    {
        DEBUG_ERROR("[FileDialog] Failed to create prefab at '%s'.", savePath.string().c_str());
        return;
    }

    PrefabData instanceData;
    instanceData.m_sourcePath = savePath;
    instanceData.m_name = savePath.stem().string();
    instanceData.m_prefabUID = m_go->GetID();
    PrefabManager::linkInstance(m_go, instanceData);

    app->getModuleAssets()->refresh();
}
