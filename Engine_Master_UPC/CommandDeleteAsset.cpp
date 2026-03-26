#include "Globals.h"
#include "CommandDeleteAsset.h"

#include "Application.h"
#include "ModuleFileSystem.h"

CommandDeleteAsset::CommandDeleteAsset(const std::filesystem::path& metaPath)
    : m_metaPath(metaPath)
{
}

void CommandDeleteAsset::run()
{
    ModuleFileSystem* fs = app->getModuleFileSystem();

    const std::filesystem::path sourcePath = m_metaPath.parent_path() / m_metaPath.stem();
    const bool deletedMeta = fs->remove(m_metaPath);
    const bool deletedSource = fs->remove(sourcePath);
    m_result = deletedMeta && deletedSource;
}

bool CommandDeleteAsset::getResult() const
{
    return m_result;
}
