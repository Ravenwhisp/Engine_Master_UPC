#pragma once
#include "ICommand.h"

#include <filesystem>

class CommandDeleteFolder : public ICommand
{
public:
    CommandDeleteFolder(const std::filesystem::path& folderPath, const std::filesystem::path& currentDirectory);

    void run() override;
    std::filesystem::path getResult() const;

private:
    std::filesystem::path m_folderPath;
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_result;
};
