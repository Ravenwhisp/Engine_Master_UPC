#pragma once
#include <filesystem>


class DeleteAssetAction
{
public:
    explicit DeleteAssetAction(const std::filesystem::path& metaPath);

    // Returns true when both removals succeeded.
    bool run();

private:
    std::filesystem::path m_metaPath;
};