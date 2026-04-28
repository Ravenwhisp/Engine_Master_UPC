#pragma once
#include "ICommand.h"

#include <filesystem>

struct FileDialogClipboard;

class CommandPasteFile : public ICommand
{
public:
    CommandPasteFile(FileDialogClipboard& clipboard, const std::filesystem::path& targetDir);

    void run() override;

private:
    FileDialogClipboard& m_clipboard;
    std::filesystem::path m_targetDir;
};
