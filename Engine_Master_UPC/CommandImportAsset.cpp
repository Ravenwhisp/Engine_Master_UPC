#include "Globals.h"
#include "CommandImportAsset.h"

#include "Application.h"
#include "ModuleAssets.h"

CommandImportAsset::CommandImportAsset(const std::filesystem::path& sourcePath,
    UID uid)
    : m_sourcePath(sourcePath)
    , m_uid(uid)
{
}

void CommandImportAsset::run()
{
    AssetId ref(m_uid);
    app->getModuleAssets()->importAsset(m_sourcePath, ref);
}
