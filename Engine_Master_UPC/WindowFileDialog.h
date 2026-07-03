#pragma once

#include "EditorWindow.h"
#include "FileDialogClipboard.h"
#include "PrefabUI.h"
#include "UID.h"

#include "AssetType.h"
#include "Extensions.h"

#include <filesystem>
#include <unordered_set>

#include "IconsFontAwesome5.h"

struct AssetEntry;
struct DirectoryEntry;

struct AssetUIProperties
{
    const char* iconGlyph;
    const char* payloadID;
};

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

    inline AssetType getAssetType(std::string_view extension)
    {
        if (extension == PREFAB_EXTENSION)                  return AssetType::PREFAB;
        if (extension == MESH_EXTENSION)                    return AssetType::MESH;
        if (extension == MATERIAL_EXTENSION)                return AssetType::MATERIAL;
        if (extension == ANIMATION_EXTENSION)               return AssetType::ANIMATION;
        if (extension == ANIMATION_STATE_MACHINE_EXTENSION) return AssetType::ANIMATION_STATE_MACHINE;
        if (extension == SCENE_EXTENSION)                   return AssetType::SCENE;
        if (extension == GLTF_EXTENSION)                    return AssetType::MODEL;

        if (extension == CPP_EXTENSION || extension == H_EXTENSION)
        {
            return AssetType::SCRIPT;
        }

        if (extension == PNG_EXTENSION ||
            extension == JPG_EXTENSION ||
            extension == JPEG_EXTENSION ||
            extension == BMP_EXTENSION ||
            extension == TGA_EXTENSION ||
            extension == DDS_EXTENSION ||
            extension == HDR_EXTENSION)
        {
            return AssetType::TEXTURE;
        }

        return AssetType::UNKNOWN;
    }

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