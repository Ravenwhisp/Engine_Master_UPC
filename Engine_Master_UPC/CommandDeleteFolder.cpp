#include "Globals.h"
#include "CommandDeleteFolder.h"

#include "Application.h"
#include "ModuleAssets.h"

CommandDeleteFolder::CommandDeleteFolder(const std::filesystem::path& folderPath,
    const std::filesystem::path& currentDirectory)
    : m_folderPath(folderPath)
    , m_currentDirectory(currentDirectory)
{
}

void CommandDeleteFolder::run()
{
    if (!std::filesystem::exists(m_folderPath))
    {
        m_result = std::filesystem::path{};
        return;
    }

    std::filesystem::remove_all(m_folderPath);

    app->getModuleAssets()->refresh();

    const std::string currentStr = m_currentDirectory.string();
    const std::string deletedStr = m_folderPath.string();

    const bool needsRedirect = (m_currentDirectory == m_folderPath) || (currentStr.find(deletedStr) == 0);

    m_result = needsRedirect ? m_folderPath.parent_path() : std::filesystem::path{};
}

std::filesystem::path CommandDeleteFolder::getResult() const
{
    return m_result;
}
