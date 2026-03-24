#pragma once
#include <filesystem>
#include <MD5Fwd.h>

class ImportAssetAction
{
public:
    // uid is the asset's MD5Hash identifier.
    ImportAssetAction(const std::filesystem::path& sourcePath, MD5Hash uid);

    void run();

private:
    std::filesystem::path m_sourcePath;
    MD5Hash              m_uid;
};