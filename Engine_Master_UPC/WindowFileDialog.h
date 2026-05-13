#pragma once

#include "EditorWindow.h"
#include "FileDialogClipboard.h"
#include "PrefabUI.h"
#include "UID.h"

#include <filesystem>

struct AssetEntry;
struct DirectoryEntry;

class WindowFileDialog : public EditorWindow
{
public:
    void drawInternal() override;

    const char* getWindowName() const override
    {
        return "FileDialog";
    }

private:
    void drawDirectoryTree(DirectoryEntry* directory);
    void drawAssetGrid(DirectoryEntry* directory);

    void drawDirectoryItem(DirectoryEntry* directory);
    void drawAssetItem(DirectoryEntry* directory, const AssetEntry& asset);

    void navigateTo(const std::filesystem::path& path);

    void handleAssetClick(const AssetEntry& asset);
    void handleDirectoryClick(DirectoryEntry* directory);
    void handleDirectoryDoubleClick(DirectoryEntry* directory);
    void handleGameObjectDrop(const std::filesystem::path& targetDirectory);

    std::filesystem::path getAssetSourcePath(
        const DirectoryEntry& directory,
        const AssetEntry& asset
    ) const;

    std::filesystem::path getAssetMetaPath(
        const DirectoryEntry& directory,
        const AssetEntry& asset
    ) const;

    PrefabUI::FileDialogBuffers buildFileDialogBuffers();

private:
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_selectedPath;
    UID m_selectedAsset = INVALID_UID;

    FileDialogClipboard m_clipboard;

    bool m_showVariantModal = false;
    bool m_showSavePrefabModal = false;
    bool m_renamingPrefab = false;

    char m_variantSrcBuf[128] = {};
    char m_variantDstBuf[128] = {};
    char m_savePrefabNameBuf[128] = {};
    char m_renameSrcBuf[128] = {};
    char m_renameDstBuf[128] = {};
};