#include "Globals.h"
#include "ImportAssetAction.h"

#include "Application.h"
#include "ModuleAssets.h"

ImportAssetAction::ImportAssetAction(const std::filesystem::path& sourcePath,
    MD5Hash uid)
    : m_sourcePath(sourcePath)
    , m_uid(uid)
{
}

void ImportAssetAction::run()
{
    app->getModuleAssets()->importAsset(m_sourcePath, m_uid);
}