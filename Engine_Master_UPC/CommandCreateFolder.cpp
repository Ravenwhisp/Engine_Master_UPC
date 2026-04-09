#include "Globals.h"
#include "CommandCreateFolder.h"

#include "Application.h"
#include "ModuleAssets.h"

CommandCreateFolder::CommandCreateFolder(const std::filesystem::path& parentDir)
    : m_parentDir(parentDir)
{
}

void CommandCreateFolder::run()
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
