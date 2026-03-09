#pragma once
#include "imgui.h"
#include <string>

class GameObject;
class SceneModule;

class PrefabUI
{
public:
    static void drawModeHeader(const char* prefabName);
    static void drawApplyRevertBar(float availableWidth);
    static void drawInstanceBadge(const GameObject* go);
    static void drawSavePrefabSection(GameObject* go);
    static void drawNodeContextMenu(GameObject* go, bool prefabMode, bool isEditRoot);
    static void drawPrefabSubMenu(GameObject* go, SceneModule* scene);
    static void drawExitOverlay(ImVec2 viewportPos, ImVec2 viewportSize);
    static void markTransformOverride(GameObject* go);
    static void drawFileDialogInstanceBar(GameObject* go);

    struct FileDialogBuffers
    {
        char* variantSource;      int variantSourceSize;
        char* variantDest;        int variantDestSize;
        char* renameSource;       int renameSourceSize;
        char* renameDest;         int renameDestSize;
        char* savePrefab;         int savePrefabSize;
    };

    static void drawFileDialogItemContextMenu(const std::string& prefabName,
        bool& outShowVariantModal,
        bool& outRenamingPrefab,
        FileDialogBuffers& buffers);

    static void drawFileDialogModals(bool& showVariantModal,
        bool& showSavePrefabModal,
        bool& renamingPrefab,
        FileDialogBuffers& buffers);
};