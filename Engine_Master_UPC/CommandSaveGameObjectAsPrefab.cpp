#include "Globals.h"
#include "CommandSaveGameObjectAsPrefab.h"


#include "Application.h"
#include "ModuleAssets.h"

#include "GameObject.h"
#include <PrefabManager.h>
#include "PrefabAsset.h"
#include <Extensions.h>
#include <FileIO.h>

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
    while (FileIO::exists(savePath))
    {
        savePath = m_targetDir / (m_go->GetName() + "_" + std::to_string(suffix++) + PREFAB_EXTENSION);
    }

    if (!PrefabManager::createPrefab(m_go, savePath))
    {
        DEBUG_ERROR("[FileDialog] Failed to create prefab at '%s'.", savePath.string().c_str());
        return;
    }

    app->getModuleAssets()->refresh();
}
