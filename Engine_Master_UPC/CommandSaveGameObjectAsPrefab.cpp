#include "Globals.h"
#include "CommandSaveGameObjectAsPrefab.h"

#include "Application.h"
#include "ModuleAssets.h"

#include "GameObject.h"
#include "Prefab.h"
#include "PrefabInstanceComponent.h"
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

    AssetId ref;
    Prefab tempPrefab(ref);
    tempPrefab.setUID(GenerateUID());
    tempPrefab.buildFrom(m_go);
    tempPrefab.m_sourcePath = savePath;

    if (!app->getModuleAssets()->save(tempPrefab, savePath))
    {
        DEBUG_ERROR("[FileDialog] Failed to create prefab at '%s'.", savePath.string().c_str());
        return;
    }

    auto* preComp = static_cast<PrefabInstanceComponent*>(
        m_go->AddComponentWithUID(ComponentType::PREFAB_INSTANCE, GenerateUID()));
    if (preComp)
    {
        preComp->getData().m_sourcePath = savePath;
        const UID assetUID = app->getModuleAssets()->getIndex().findUID(savePath);
        if (isValidUID(assetUID))
            preComp->getData().m_assetUID = assetUID;
    }

    const UID existingUID = app->getModuleAssets()->getIndex().findUID(savePath);
    if (isValidUID(existingUID))
        app->getModuleAssets()->unload(AssetId(existingUID));
}
