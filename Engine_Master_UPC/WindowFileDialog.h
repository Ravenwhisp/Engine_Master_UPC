#pragma once

#include "EditorWindow.h"
#include "FileDialogClipboard.h"
#include "PrefabUI.h"
#include "UID.h"

#include <filesystem>
#include <unordered_set>

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
    void drawSubAssetItem(const AssetEntry& subAsset);

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
    std::unordered_set<UID> m_expandedAssets;

    bool m_showVariantModal = false;
    bool m_showSavePrefabModal = false;
    bool m_renamingPrefab = false;

    std::filesystem::path m_pendingStateMachinePath;

    char m_variantSrcBuf[512] = {};
    char m_variantDstBuf[512] = {};
    char m_savePrefabNameBuf[512] = {};
    char m_renameSrcBuf[512] = {};
    char m_renameDstBuf[512] = {};
};