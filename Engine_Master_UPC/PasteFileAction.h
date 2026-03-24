#pragma once
#include <filesystem>

class FileDialogClipboard;

class PasteFileAction
{
public:
    PasteFileAction(FileDialogClipboard& clipboard, const std::filesystem::path& targetDir);

    void run();

private:
    FileDialogClipboard& m_clipboard;
    std::filesystem::path m_targetDir;
};