#pragma once
#include "EditorWindow.h"
#include <filesystem>
#include "FileSystemModule.h"

enum Command {
    NONE,
    MOVE,
    COPY
};

class FileDialog : public EditorWindow {
public:
    FileDialog();
    void render() override;
    const char* getWindowName() const override { return "FileDialog"; }

private:
    void drawDirectoryTree(const std::shared_ptr<FileEntry> entry);
    void drawAssetGrid(const std::shared_ptr<FileEntry> directory);

    bool moveFile(FileEntry* targetDirectory);

    std::filesystem::path m_currentDirectory;
    std::shared_ptr<FileEntry> m_selectedItem;

    Command m_lastActionRequested = Command::NONE;
    std::filesystem::path m_fileToManage;
};
