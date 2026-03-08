#include "Globals.h"
#include "PrefabUI.h"

#include "Application.h"
#include "EditorModule.h"
#include "SceneModule.h"
#include "PrefabManager.h"
#include "PrefabEditSession.h"
#include "GameObject.h"
#include "ComponentType.h"
#include "FileSystemModule.h"

void PrefabUI::drawModeHeader(const char* prefabName)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 p2 = { p.x + ImGui::GetContentRegionAvail().x, p.y + 28.f };
    ImGui::GetWindowDrawList()->AddRectFilled(p, p2, IM_COL32(25, 80, 25, 210));
    ImGui::GetWindowDrawList()->AddRect(p, p2, IM_COL32(50, 190, 50, 180));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.f, 0.35f, 1.f));
    ImGui::Text("  Prefab: %s", prefabName);
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Separator();
}

void PrefabUI::drawApplyRevertBar(float availableWidth)
{
    PrefabEditSession* session = app->getEditorModule()->getPrefabSession();
    if (!session || !session->active || !session->rootObject) return;

    const float bw = (availableWidth - 8.f) / 3.f;
    const PrefabInstanceData* inst = PrefabManager::getInstanceData(session->rootObject);
    const bool hasChanges = inst && !inst->overrides.isEmpty();

    ImGui::BeginDisabled(!hasChanges);
    ImGui::PushStyleColor(ImGuiCol_Button, hasChanges ? ImVec4(0.14f, 0.42f, 0.14f, 1.f)
        : ImVec4(0.15f, 0.15f, 0.15f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.58f, 0.20f, 1.f));
    if (ImGui::Button("Apply", ImVec2(bw, 0)))
    {
        PrefabManager::applyToPrefab(session->rootObject);
        app->getEditorModule()->exitPrefabEdit();
    }
    ImGui::PopStyleColor(2);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(hasChanges ? "Save changes to prefab file" : "No changes to apply");

    ImGui::SameLine(0, 4);

    ImGui::BeginDisabled(!hasChanges);
    if (ImGui::Button("Revert", ImVec2(bw, 0)))
    {
        PrefabManager::revertToPrefab(session->rootObject, session->isolatedScene.get());
        app->getEditorModule()->setSelectedGameObject(session->rootObject);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(hasChanges ? "Reload from prefab file" : "No changes to revert");

    ImGui::SameLine(0, 4);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.12f, 0.12f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.17f, 0.17f, 1.f));
    if (ImGui::Button("Exit", ImVec2(bw, 0)))
        app->getEditorModule()->exitPrefabEdit();
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Leave without saving  [Esc]");

    ImGui::Spacing();
    ImGui::Separator();
}

void PrefabUI::drawInstanceBadge(const GameObject* go)
{
    if (!PrefabManager::isPrefabInstance(go)) return;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.75f, 1.0f, 1.f));
    ImGui::Text("[Prefab: %s]", PrefabManager::getPrefabName(go).c_str());
    ImGui::PopStyleColor();

    const PrefabInstanceData* inst = PrefabManager::getInstanceData(go);
    if (inst && !inst->overrides.isEmpty())
    {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.1f, 1.f));
        ImGui::Text("(overrides)");
        ImGui::PopStyleColor();
    }

    ImGui::Separator();
}

void PrefabUI::drawSavePrefabSection(GameObject* go)
{
    if (!go) return;

    ImGui::Spacing();
    ImGui::SeparatorText("Prefab");

    static char buf[256] = "";
    ImGui::SetNextItemWidth(-80);
    ImGui::InputText("##prefabname", buf, sizeof(buf));
    ImGui::SameLine();

    if (ImGui::Button("Save") && strlen(buf) > 0)
    {
        if (PrefabManager::createPrefab(go, buf))
        {
            PrefabInstanceData d;
            d.prefabName = buf;
            d.prefabUID = PrefabManager::makePrefabUID(buf);
            PrefabManager::linkInstance(go, d);
            buf[0] = '\0';
        }
    }

    if (PrefabManager::isPrefabInstance(go))
    {
        ImGui::Spacing();
        if (ImGui::Button("Edit Prefab"))
            app->getEditorModule()->enterPrefabEdit(PrefabManager::getPrefabName(go));
    }
}

void PrefabUI::drawNodeContextMenu(GameObject* go, bool prefabMode, bool isEditRoot)
{
    if (!prefabMode) return;

    PrefabEditSession* session = app->getEditorModule()->getPrefabSession();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.75f, 0.2f, 1.f));
    ImGui::Text("Prefab: %s", session->prefabName.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    const PrefabInstanceData* inst = PrefabManager::getInstanceData(session->rootObject);
    const bool hasChanges = inst && !inst->overrides.isEmpty();

    ImGui::BeginDisabled(!hasChanges);
    ImGui::PushStyleColor(ImGuiCol_Text, hasChanges ? ImVec4(0.3f, 1.f, 0.3f, 1.f)
        : ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    if (ImGui::MenuItem("Apply  -  Save changes to prefab file"))
    {
        PrefabManager::applyToPrefab(session->rootObject);
        app->getEditorModule()->exitPrefabEdit();
    }
    ImGui::PopStyleColor();
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasChanges)
        ImGui::SetTooltip("No changes to apply");

    ImGui::BeginDisabled(!hasChanges);
    if (ImGui::MenuItem("Revert  -  Reload from prefab file"))
    {
        PrefabManager::revertToPrefab(session->rootObject, session->isolatedScene.get());
        app->getEditorModule()->setSelectedGameObject(session->rootObject);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasChanges)
        ImGui::SetTooltip("No changes to revert");

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
    if (ImGui::MenuItem("Unlink  -  Break prefab connection"))
    {
        PrefabManager::unlinkInstance(session->rootObject);
        app->getEditorModule()->exitPrefabEdit();
    }
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Exit Prefab Edit"))
        app->getEditorModule()->exitPrefabEdit();
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void PrefabUI::drawPrefabSubMenu(GameObject* go, SceneModule* scene)
{
    if (!go || !ImGui::BeginMenu("Prefab")) return;

    static char pfBuf[128] = "";
    ImGui::SetNextItemWidth(140.f);
    ImGui::InputTextWithHint("##pfn", go->GetName().c_str(), pfBuf, sizeof(pfBuf));
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        std::string name = strlen(pfBuf) > 0 ? pfBuf : go->GetName();
        if (PrefabManager::createPrefab(go, name))
        {
            PrefabInstanceData d;
            d.prefabName = name;
            d.prefabUID = PrefabManager::makePrefabUID(name);
            PrefabManager::linkInstance(go, d);
        }
        memset(pfBuf, 0, sizeof(pfBuf));
    }

    if (PrefabManager::isPrefabInstance(go))
    {
        ImGui::Separator();
        if (ImGui::MenuItem("Edit Prefab"))
            app->getEditorModule()->enterPrefabEdit(PrefabManager::getPrefabName(go));
        if (ImGui::MenuItem("Apply to Prefab"))
            PrefabManager::applyToPrefab(go);
        if (ImGui::MenuItem("Revert to Prefab"))
            PrefabManager::revertToPrefab(go, scene);
        ImGui::Separator();
        if (ImGui::MenuItem("Unlink"))
            PrefabManager::unlinkInstance(go);
    }

    ImGui::EndMenu();
}

void PrefabUI::drawExitOverlay(ImVec2 viewportPos, ImVec2 viewportSize)
{
    PrefabEditSession* session = app->getEditorModule()->getPrefabSession();
    if (!session || !session->active) return;

    constexpr ImGuiWindowFlags kFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoSavedSettings;

    constexpr float kBtnW = 170.f;
    ImGui::SetNextWindowPos(
        { viewportPos.x + viewportSize.x - kBtnW - 10.f, viewportPos.y + 28.f },
        ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);

    if (!ImGui::Begin("##pfExit", nullptr, kFlags)) { ImGui::End(); return; }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.20f, 1.f));
    ImGui::Text("Editing: %s", session->prefabName.c_str());
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.18f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.38f, 0.07f, 0.07f, 1.f));
    if (ImGui::Button("Exit Prefab Edit  [Esc]", ImVec2(kBtnW, 0)))
        app->getEditorModule()->exitPrefabEdit();
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Leave without saving. Right-click in Hierarchy to Apply/Revert.");

    ImGui::End();
}

void PrefabUI::markTransformOverride(GameObject* go)
{
    if (!go) return;

    GameObject* cur = go;
    while (cur)
    {
        if (PrefabManager::isPrefabInstance(cur))
        {
            PrefabManager::markPropertyOverride(cur, static_cast<int>(ComponentType::TRANSFORM), "position");
            PrefabManager::markPropertyOverride(cur, static_cast<int>(ComponentType::TRANSFORM), "rotation");
            PrefabManager::markPropertyOverride(cur, static_cast<int>(ComponentType::TRANSFORM), "scale");
            return;
        }
        Transform* root = cur->GetTransform()->getRoot();
        cur = root ? root->getOwner() : nullptr;
    }
}

void PrefabUI::drawFileDialogInstanceBar(GameObject* go)
{
    if (!go || !PrefabManager::isPrefabInstance(go)) return;

    const std::string pfName = PrefabManager::getPrefabName(go);
    const PrefabInstanceData* inst = PrefabManager::getInstanceData(go);
    const bool hasOverrides = inst && !inst->overrides.isEmpty();

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 p2 = { p.x + ImGui::GetContentRegionAvail().x, p.y + 52.f };
    ImGui::GetWindowDrawList()->AddRectFilled(p, p2, IM_COL32(20, 60, 20, 180));
    ImGui::GetWindowDrawList()->AddRect(p, p2, IM_COL32(50, 160, 50, 140));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.f, 0.35f, 1.f));
    ImGui::Text("  [P]");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);
    ImGui::Text("%s", go->GetName().c_str());
    ImGui::SameLine(0, 4);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
    ImGui::Text("->  %s", pfName.c_str());
    ImGui::PopStyleColor();
    if (hasOverrides)
    {
        ImGui::SameLine(0, 4);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.1f, 1.f));
        ImGui::Text("(overrides)");
        ImGui::PopStyleColor();
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.f);
    const float pad = 4.f;

    ImGui::BeginDisabled(!hasOverrides);
    ImGui::PushStyleColor(ImGuiCol_Button,
        hasOverrides ? ImVec4(0.14f, 0.42f, 0.14f, 1.f) : ImVec4(0.15f, 0.15f, 0.15f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.58f, 0.20f, 1.f));
    if (ImGui::SmallButton("Apply"))
    {
        PrefabManager::applyToPrefab(go);
        DEBUG_LOG("[FileDialog] Applied prefab overrides for '%s'", go->GetName().c_str());
    }
    ImGui::PopStyleColor(2);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(hasOverrides ? "Save changes to prefab file" : "No changes to apply");

    ImGui::SameLine(0, pad);

    ImGui::BeginDisabled(!hasOverrides);
    if (ImGui::SmallButton("Revert"))
    {
        PrefabManager::revertToPrefab(go, app->getSceneModule());
        app->getEditorModule()->setSelectedGameObject(go);
        DEBUG_LOG("[FileDialog] Reverted '%s' to prefab", go->GetName().c_str());
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(hasOverrides ? "Reload from prefab file" : "No changes to revert");

    ImGui::SameLine(0, pad);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
    if (ImGui::SmallButton("Unlink"))
    {
        PrefabManager::unlinkInstance(go);
        DEBUG_LOG("[FileDialog] Unlinked instance '%s'", go->GetName().c_str());
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Break prefab connection");

    ImGui::SameLine(0, pad);

    if (ImGui::SmallButton("Edit Prefab"))
        app->getEditorModule()->enterPrefabEdit(pfName);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Open prefab editor");

    ImGui::Spacing();
    ImGui::Separator();
}

void PrefabUI::drawFileDialogItemContextMenu(const std::string& prefabName,
    bool& outShowVariantModal,
    bool& outRenamingPrefab,
    char* variantSrcBuf, int variantSrcBufSize,
    char* variantDstBuf, int variantDstBufSize,
    char* renameSrcBuf, int renameSrcBufSize,
    char* renameDstBuf, int renameDstBufSize)
{
    if (!ImGui::BeginPopupContextItem("ItemContext")) return;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.f, 0.35f, 1.f));
    ImGui::Text("[P]  %s", prefabName.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (ImGui::MenuItem("Add to Scene"))
    {
        SceneModule* scene = app->getSceneModule();
        if (scene)
        {
            GameObject* go = PrefabManager::instantiatePrefab(prefabName, scene);
            if (go)
            {
                app->getEditorModule()->setSelectedGameObject(go);
                DEBUG_LOG("[FileDialog] Instantiated prefab '%s'", prefabName.c_str());
            }
            else
            {
                DEBUG_ERROR("[FileDialog] Failed to instantiate prefab '%s'", prefabName.c_str());
            }
        }
    }

    if (ImGui::MenuItem("Edit Prefab..."))
        app->getEditorModule()->enterPrefabEdit(prefabName);

    ImGui::Separator();

    GameObject* selected = app->getEditorModule()->getSelectedGameObject();
    const bool isLinked = selected
        && PrefabManager::isPrefabInstance(selected)
        && PrefabManager::getPrefabName(selected) == prefabName;
    const PrefabInstanceData* inst = isLinked ? PrefabManager::getInstanceData(selected) : nullptr;
    const bool hasOverrides = inst && !inst->overrides.isEmpty();

    ImGui::BeginDisabled(!hasOverrides);
    if (ImGui::MenuItem("Apply to Prefab"))
    {
        PrefabManager::applyToPrefab(selected);
        DEBUG_LOG("[FileDialog] Applied overrides for '%s'", prefabName.c_str());
    }
    if (ImGui::MenuItem("Revert to Prefab"))
    {
        PrefabManager::revertToPrefab(selected, app->getSceneModule());
        app->getEditorModule()->setSelectedGameObject(selected);
        DEBUG_LOG("[FileDialog] Reverted '%s' to prefab", prefabName.c_str());
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasOverrides)
        ImGui::SetTooltip("Select a linked instance first");

    ImGui::Separator();

    if (ImGui::MenuItem("Create Variant..."))
    {
        outShowVariantModal = true;
        strncpy_s(variantSrcBuf, variantSrcBufSize, prefabName.c_str(), variantSrcBufSize - 1);
        snprintf(variantDstBuf, variantDstBufSize, "%s_variant", prefabName.c_str());
    }

    if (ImGui::MenuItem("Rename..."))
    {
        outRenamingPrefab = true;
        strncpy_s(renameSrcBuf, renameSrcBufSize, prefabName.c_str(), renameSrcBufSize - 1);
        strncpy_s(renameDstBuf, renameDstBufSize, prefabName.c_str(), renameDstBufSize - 1);
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Delete Prefab File"))
    {
        std::string path = std::string("Assets/Prefabs/") + prefabName + ".prefab";
        if (app->getFileSystemModule()->exists(path.c_str()))
        {
            app->getFileSystemModule()->deleteFile(path.c_str());
            app->getFileSystemModule()->rebuild();
            DEBUG_LOG("[FileDialog] Deleted prefab '%s'", prefabName.c_str());
        }
    }
    ImGui::PopStyleColor();

    ImGui::EndPopup();
}

void PrefabUI::drawFileDialogModals(bool& showVariantModal,
    bool& showSavePrefabModal,
    bool& renamingPrefab,
    char* variantSrcBuf, int variantSrcBufSize,
    char* variantDstBuf, int variantDstBufSize,
    char* savePrefabBuf, int savePrefabBufSize,
    char* renameSrcBuf, int renameSrcBufSize,
    char* renameDstBuf, int renameDstBufSize)
{
    ImVec2 centre = ImGui::GetMainViewport()->GetCenter();

    if (showVariantModal) { ImGui::OpenPopup("##pfVariantModal"); showVariantModal = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 320, 0 });
    if (ImGui::BeginPopupModal("##pfVariantModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Create variant of:  \"%s\"", variantSrcBuf);
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("Variant name##vname", variantDstBuf, variantDstBufSize);
        ImGui::Spacing();
        if (ImGui::Button("Create", { 100, 0 }))
        {
            if (strlen(variantDstBuf) > 0)
            {
                if (PrefabManager::createVariant(variantSrcBuf, variantDstBuf))
                {
                    app->getFileSystemModule()->rebuild();
                    DEBUG_LOG("[FileDialog] Created variant '%s' from '%s'", variantDstBuf, variantSrcBuf);
                }
                else
                {
                    DEBUG_ERROR("[FileDialog] Variant creation failed: %s -> %s", variantSrcBuf, variantDstBuf);
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 80, 0 })) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (renamingPrefab) { ImGui::OpenPopup("##pfRenameModal"); renamingPrefab = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 300, 0 });
    if (ImGui::BeginPopupModal("##pfRenameModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Rename prefab:  \"%s\"", renameSrcBuf);
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        bool enter = ImGui::InputText("New name##rname", renameDstBuf, renameDstBufSize,
            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if (ImGui::Button("Rename", { 100, 0 }) || enter)
        {
            if (strlen(renameDstBuf) > 0 && strcmp(renameSrcBuf, renameDstBuf) != 0)
            {
                std::string oldPath = std::string("Assets/Prefabs/") + renameSrcBuf + ".prefab";
                std::string newPath = std::string("Assets/Prefabs/") + renameDstBuf + ".prefab";
                if (app->getFileSystemModule()->move(oldPath.c_str(), newPath.c_str()))
                {
                    app->getFileSystemModule()->rebuild();
                    DEBUG_LOG("[FileDialog] Renamed prefab '%s' -> '%s'", renameSrcBuf, renameDstBuf);
                }
                else
                {
                    DEBUG_ERROR("[FileDialog] Rename failed: %s -> %s", renameSrcBuf, renameDstBuf);
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 80, 0 })) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (showSavePrefabModal) { ImGui::OpenPopup("##pfSaveModal"); showSavePrefabModal = false; }
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 300, 0 });
    if (ImGui::BeginPopupModal("##pfSaveModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        GameObject* sel = app->getEditorModule()->getSelectedGameObject();
        ImGui::Text("Save \"%s\" as prefab:", sel ? sel->GetName().c_str() : "?");
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        bool enter = ImGui::InputText("Prefab name##pfn", savePrefabBuf, savePrefabBufSize,
            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if ((ImGui::Button("Save", { 100, 0 }) || enter) && strlen(savePrefabBuf) > 0)
        {
            if (sel && PrefabManager::createPrefab(sel, savePrefabBuf))
            {
                PrefabInstanceData d;
                d.prefabName = savePrefabBuf;
                d.prefabUID = PrefabManager::makePrefabUID(savePrefabBuf);
                PrefabManager::linkInstance(sel, d);
                app->getFileSystemModule()->rebuild();
                DEBUG_LOG("[FileDialog] Saved prefab '%s'", savePrefabBuf);
            }
            else
            {
                DEBUG_ERROR("[FileDialog] Prefab save failed for '%s'", savePrefabBuf);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 80, 0 })) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}