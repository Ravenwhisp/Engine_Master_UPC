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
struct ScriptSourceInfo;

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

    void drawScriptsTreeNode();
    void drawScriptGrid();
    void drawScriptItem(const ScriptSourceInfo& script);

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

    bool passesSearchFilter(const std::string& name) const;

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
    bool m_viewingScripts = false;
    char m_searchBuffer[128] = {};

    FileDialogClipboard m_clipboard;
    std::unordered_set<UID> m_expandedAssets;

    const std::unordered_map<AssetType, AssetUIProperties> assetUIData = {
    { AssetType::TEXTURE,                 { ICON_FA_IMAGE,          "DND_TEXTURE" } },
    { AssetType::MODEL,                   { ICON_FA_CUBES,          "DND_MODEL" } },
    { AssetType::MATERIAL,                { ICON_FA_PALETTE,        "DND_MATERIAL" } },
    { AssetType::MESH,                    { ICON_FA_PROJECT_DIAGRAM,"DND_MESH" } },
    { AssetType::FONT,                    { ICON_FA_FONT,           "DND_FONT" } },
    { AssetType::PREFAB,                  { ICON_FA_CUBE,           "DND_PREFAB" } },
    { AssetType::ANIMATION,               { ICON_FA_RUNNING,        "DND_ANIMATION" } },
    { AssetType::SKIN,                    { ICON_FA_USER_TAG,       "DND_SKIN" } },
    { AssetType::ANIMATION_STATE_MACHINE, { ICON_FA_SITEMAP,        "DND_STATE_MACHINE" } },
    { AssetType::SCENE,                   { ICON_FA_FILM,           "DND_SCENE" } },
    { AssetType::SCRIPT,                  { ICON_FA_FILE_CODE,      "SCRIPT_ASSET" } },
    { AssetType::UNKNOWN,                 { ICON_FA_FILE,           "DND_UNKNOWN" } }
    };

    bool m_showVariantModal = false;
    bool m_showSavePrefabModal = false;
    bool m_renamingPrefab = false;
    bool m_renamingAsset = false;

    std::filesystem::path m_pendingStateMachinePath;

    char m_variantSrcBuf[512] = {};
    char m_variantDstBuf[512] = {};
    char m_savePrefabNameBuf[512] = {};
    char m_renameSrcBuf[512] = {};
    char m_renameDstBuf[512] = {};
};