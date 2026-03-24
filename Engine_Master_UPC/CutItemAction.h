#pragma once
#include <filesystem>

class FileDialogClipboard;

class CutItemAction
{
public:
    CutItemAction(FileDialogClipboard& clipboard, const std::filesystem::path& itemPath);

    void run();

private:
    FileDialogClipboard& m_clipboard;
    std::filesystem::path m_itemPath;
};