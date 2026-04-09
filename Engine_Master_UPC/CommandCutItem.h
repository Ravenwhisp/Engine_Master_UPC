#pragma once
#include "ICommand.h"

#include <filesystem>

class FileDialogClipboard;

class CommandCutItem : public ICommand
{
public:
    CommandCutItem(FileDialogClipboard& clipboard, const std::filesystem::path& itemPath);

    void run() override;

private:
    FileDialogClipboard& m_clipboard;
    std::filesystem::path m_itemPath;
};
