#pragma once
#include <filesystem>

class DeleteFolderAction
{
public:
    DeleteFolderAction(const std::filesystem::path& folderPath, const std::filesystem::path& currentDirectory);

    // Returns new navigation target, or empty path if unchanged.
    std::filesystem::path run();

private:
    std::filesystem::path m_folderPath;
    std::filesystem::path m_currentDirectory;
};