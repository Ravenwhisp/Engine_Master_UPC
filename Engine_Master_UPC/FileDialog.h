#pragma once
#include "EditorWindow.h"
#include <filesystem>
#include "FileSystemModule.h"

class FileDialog : public EditorWindow {
public:
    FileDialog();
    void render() override;
    const char* getWindowName() const override { return "FileDialog"; }

private:
    void drawDirectoryTree(const std::shared_ptr<FileEntry> entry);
    void drawAssetGrid(const std::shared_ptr<FileEntry> directory);

    std::filesystem::path* m_currentDirectory;
    std::shared_ptr<FileEntry> m_selectedItem;
};
