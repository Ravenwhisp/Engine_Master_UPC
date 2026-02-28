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
    void render() override;
    const char* getWindowName() const override { return "FileDialog"; }

private:
    void drawDirectoryTree(const std::shared_ptr<FileEntry> entry);
    void drawAssetGrid(const std::shared_ptr<FileEntry> directory);

    // Actions
    bool moveFile(FileEntry* targetDirectory);
    bool deleteAsset(FileEntry* file);
    void createNewFolder();
    void pasteFile(const std::shared_ptr<FileEntry>& directory);
    void importAsset(const std::shared_ptr<FileEntry>& asset);
    void cutItem(const std::shared_ptr<FileEntry>& asset);
    void deleteItem (const std::shared_ptr<FileEntry>& asset);
    void deleteFolder(const std::shared_ptr<FileEntry>& asset);

    // Navigation
    void navigateTo(const std::filesystem::path& path);
    void handleAssetDoubleClick(const std::shared_ptr<FileEntry>& asset);

    std::filesystem::path m_currentDirectory;
    std::shared_ptr<FileEntry> m_selectedItem;

    Command m_lastActionRequested = Command::NONE;
    std::filesystem::path m_fileToManage;
};