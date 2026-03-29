#pragma once
#include <filesystem>

enum class FileDialogCommand { None, Move };

struct FileDialogClipboard
{
    FileDialogCommand     command = FileDialogCommand::None;
    std::filesystem::path fileToManage;

    bool hasPending()  const { return command != FileDialogCommand::None; }
    void clear() { command = FileDialogCommand::None; fileToManage.clear(); }
};