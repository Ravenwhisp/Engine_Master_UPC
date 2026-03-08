#pragma once
#include "imgui.h"

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

    static void drawFileDialogItemContextMenu(const std::string& prefabName,
        bool& outShowVariantModal,
        bool& outRenamingPrefab,
        char* variantSrcBuf, int variantSrcBufSize,
        char* variantDstBuf, int variantDstBufSize,
        char* renameSrcBuf, int renameSrcBufSize,
        char* renameDstBuf, int renameDstBufSize);

    static void drawFileDialogModals(bool& showVariantModal,
        bool& showSavePrefabModal,
        bool& renamingPrefab,
        char* variantSrcBuf, int variantSrcBufSize,
        char* variantDstBuf, int variantDstBufSize,
        char* savePrefabBuf, int savePrefabBufSize,
        char* renameSrcBuf, int renameSrcBufSize,
        char* renameDstBuf, int renameDstBufSize);
};