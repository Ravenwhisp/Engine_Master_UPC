#pragma once
#include "filesystem"

class MoveFileAction
{
public:
    MoveFileAction(const std::filesystem::path& source, const std::filesystem::path& targetDir);

    // Returns true when every move operation succeeded.
    bool run();

private:
    std::filesystem::path m_source;
    std::filesystem::path m_targetDir;
};