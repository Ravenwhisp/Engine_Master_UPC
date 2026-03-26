#pragma once
#include "ICommand.h"

#include <filesystem>

class CommandMoveFile : public ICommand
{
public:
    CommandMoveFile(const std::filesystem::path& source, const std::filesystem::path& targetDir);

    void run() override;
    bool getResult() const;

private:
    std::filesystem::path m_source;
    std::filesystem::path m_targetDir;
    bool m_result = false;
};
