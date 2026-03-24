#pragma once
#include "EditorWindow.h"
#include "PrefabUI.h"
#include <filesystem>
#include "FileDialogClipboard.h"

struct FileEntry;

enum Command 
{
    NONE,
    MOVE,
    COPY
};

class WindowFileDialog : public EditorWindow 
{
public:
    void render() override;
    const char* getWindowName() const override { return "FileDialog"; }

private:
    void drawDirectoryTree(const std::shared_ptr<FileEntry>& entry);
    void drawAssetGrid(const std::shared_ptr<FileEntry>& directory);

    void navigateTo(const std::filesystem::path& path);
    void handleAssetDoubleClick(const std::shared_ptr<FileEntry>& asset);

    void handleGameObjectDrop(const std::filesystem::path& targetDirectory);

    PrefabUI::FileDialogBuffers buildFileDialogBuffers();

    std::filesystem::path      m_currentDirectory;
    std::shared_ptr<FileEntry> m_selectedItem;

    FileDialogClipboard        m_clipboard;

    // Prefab modal state
    bool m_showVariantModal = false;
    bool m_showSavePrefabModal = false;
    bool m_renamingPrefab = false;
    char m_variantSrcBuf[128] = {};
    char m_variantDstBuf[128] = {};
    char m_savePrefabNameBuf[128] = {};
    char m_renameSrcBuf[128] = {};
    char m_renameDstBuf[128] = {};
};