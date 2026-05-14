#pragma once
#include "ICommand.h"

#include <filesystem>
#include <UID.h>

class CommandImportAsset : public ICommand
{
public:
    CommandImportAsset(const std::filesystem::path& sourcePath, UID uid);

    void run() override;

private:
    std::filesystem::path m_sourcePath;
    UID m_uid;
};
