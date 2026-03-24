#include "Globals.h"
#include "CreateFolderAction.h"

#include "Application.h"
#include "ModuleAssets.h"

CreateFolderAction::CreateFolderAction(const std::filesystem::path& parentDir)
    : m_parentDir(parentDir)
{
}

void CreateFolderAction::run()
{
    std::filesystem::path newPath = m_parentDir / "New Folder";

    int suffix = 1;
    while (std::filesystem::exists(newPath))
    {
        newPath = m_parentDir / ("New Folder (" + std::to_string(suffix++) + ")");
    }

    std::filesystem::create_directory(newPath);
    app->getModuleAssets()->refresh();
}
