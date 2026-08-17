#pragma once
#include "ICommand.h"

#include <filesystem>
#include <string>

class CommandCreateDataContainer : public ICommand
{
public:
    CommandCreateDataContainer(const std::filesystem::path& targetDir, const std::string& typeName, const std::string& assetName);

    void run() override;

private:
    std::filesystem::path m_targetDir;
    std::string m_typeName;
    std::string m_assetName;
};