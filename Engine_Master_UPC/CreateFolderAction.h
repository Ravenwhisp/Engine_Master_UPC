#pragma once
#include <filesystem>

class CreateFolderAction
{
public:
    explicit CreateFolderAction(const std::filesystem::path& parentDir);

    void run();

private:
    std::filesystem::path m_parentDir;
};