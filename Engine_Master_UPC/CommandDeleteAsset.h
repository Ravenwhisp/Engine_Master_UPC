#pragma once
#include "ICommand.h"

#include <filesystem>

class CommandDeleteAsset : public ICommand
{
public:
    explicit CommandDeleteAsset(const std::filesystem::path& metaPath);

    void run() override;
    bool getResult() const;

private:
    std::filesystem::path m_metaPath;
    bool m_result = false;
};
