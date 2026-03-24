#include "Globals.h"
#include "DeleteFolderAction.h"

#include "Application.h"
#include "ModuleAssets.h"

DeleteFolderAction::DeleteFolderAction(const std::filesystem::path& folderPath,
    const std::filesystem::path& currentDirectory)
    : m_folderPath(folderPath)
    , m_currentDirectory(currentDirectory)
{
}

std::filesystem::path DeleteFolderAction::run()
{
    if (!std::filesystem::exists(m_folderPath))
        return {};

    std::filesystem::remove_all(m_folderPath);

    // Clear any stale clipboard reference that pointed into this folder
    // (caller is responsible for checking its own clipboard)

    app->getModuleAssets()->refresh();

    // Tell the browser to navigate up when the current directory was inside
    // the folder that was just deleted
    const std::string currentStr = m_currentDirectory.string();
    const std::string deletedStr = m_folderPath.string();

    const bool needsRedirect = (m_currentDirectory == m_folderPath) || (currentStr.find(deletedStr) == 0);

    return needsRedirect ? m_folderPath.parent_path() : std::filesystem::path{};
}
