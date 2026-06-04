#pragma once
#include "imgui.h"
#include <filesystem>
#include <string>

class GameObject;
class Scene;

class PrefabUI
{
    struct PrefabFileInfo
    {
        std::filesystem::path m_sourcePath;
        std::string m_name;
    };
public:
    static void drawModeHeader(const char* prefabName);
    static void drawApplyRevertBar(float availableWidth);

    // go is the selected object; the user types a full relative save path.
    static void drawSavePrefabSection(GameObject* go);

    static void drawNodeContextMenu(bool prefabMode);

    // scene is passed so the sub-menu can call instantiatePrefab directly.
    static void drawPrefabSubMenu(GameObject* go, Scene* scene);

    static void drawExitOverlay(ImVec2 viewportPos, ImVec2 viewportSize);
    static void markTransformOverride(GameObject* go);

    static std::vector<PrefabFileInfo> listPrefabsInfo(const std::filesystem::path& searchRoot);

    // Draws the green instance bar in the inspector.
    static void drawFileDialogInstanceBar(GameObject* go);

    // ── File-dialog context menus ───────────────────────────────────────────
    // Buffers hold full relative paths (e.g. "Assets/Chars/Hero.prefab").
    // All char* sizes should be at least 512 to accommodate path strings.
    struct FileDialogBuffers
    {
        char* variantSource;    int variantSourceSize;
        char* variantDest;      int variantDestSize;
        char* renameSource;     int renameSourceSize;
        char* renameDest;       int renameDestSize;
        char* savePrefab;       int savePrefabSize;
    };

    // sourcePath is the full path of the prefab file being right-clicked.
    static void drawFileDialogItemContextMenu(const std::filesystem::path& sourcePath,
        bool& outShowVariantModal,
        bool& outRenamingPrefab,
        FileDialogBuffers& buffers);

    static void drawFileDialogModals(bool& showVariantModal,
        bool& showSavePrefabModal,
        bool& renamingPrefab,
        bool& renamingAsset,
        FileDialogBuffers& buffers);
};