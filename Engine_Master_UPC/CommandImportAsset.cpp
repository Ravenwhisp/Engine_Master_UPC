#include "Globals.h"
#include "CommandImportAsset.h"

#include "Application.h"
#include "ModuleAssets.h"

CommandImportAsset::CommandImportAsset(const std::filesystem::path& sourcePath,
    MD5Hash uid)
    : m_sourcePath(sourcePath)
    , m_uid(uid)
{
}

void CommandImportAsset::run()
{
    app->getModuleAssets()->importAsset(m_sourcePath, m_uid);
}
