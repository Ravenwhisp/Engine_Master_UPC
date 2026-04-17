#pragma once
#include "ICommand.h"

#include <filesystem>

class CommandCreateFolder : public ICommand
{
public:
    explicit CommandCreateFolder(const std::filesystem::path& parentDir);

    void run() override;

private:
    std::filesystem::path m_parentDir;
};
