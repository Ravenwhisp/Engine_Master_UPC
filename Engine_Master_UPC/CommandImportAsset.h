#pragma once
#include "ICommand.h"

#include <filesystem>
#include <MD5Fwd.h>

class CommandImportAsset : public ICommand
{
public:
    CommandImportAsset(const std::filesystem::path& sourcePath, MD5Hash uid);

    void run() override;

private:
    std::filesystem::path m_sourcePath;
    MD5Hash m_uid;
};
