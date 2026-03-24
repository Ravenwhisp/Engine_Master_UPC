#include "Globals.h"
#include "DeleteAssetAction.h"

#include "Application.h"
#include "ModuleFileSystem.h"

DeleteAssetAction::DeleteAssetAction(const std::filesystem::path& metaPath)
    : m_metaPath(metaPath)
{
}

bool DeleteAssetAction::run()
{
    ModuleFileSystem* fs = app->getModuleFileSystem();

    const std::filesystem::path sourcePath = m_metaPath.parent_path() / m_metaPath.stem();
    const bool deletedMeta = fs->remove(m_metaPath);
    const bool deletedSource = fs->remove(sourcePath);
    return deletedMeta && deletedSource;
}
