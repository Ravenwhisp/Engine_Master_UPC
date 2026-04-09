#include "Globals.h"
#include "PrefabUI.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleAssets.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "ComponentType.h"
#include "Transform.h"

#include "PrefabManager.h"
#include "PrefabAsset.h"
#include "PrefabEditSession.h"
#include <FileIO.h>

static void linkAndSavePrefab(GameObject* go, const std::filesystem::path& savePath)
{
    PrefabManager::createPrefab(go, savePath);
}

void PrefabUI::drawModeHeader(const char* prefabName)
{
    ImVec2 topLeft = ImGui::GetCursorScreenPos();
    ImVec2 bottomRight = { topLeft.x + ImGui::GetContentRegionAvail().x, topLeft.y + 28.f };

    ImGui::GetWindowDrawList()->AddRectFilled(topLeft, bottomRight, IM_COL32(25, 80, 25, 210));
    ImGui::GetWindowDrawList()->AddRect(topLeft, bottomRight, IM_COL32(50, 190, 50, 180));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.f, 0.35f, 1.f));
    ImGui::Text("  Prefab: %s", prefabName);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
}

void PrefabUI::drawApplyRevertBar(float availableWidth)
{
    GameObject* root = app->getModuleEditor()->getPrefabEditRoot();
    if (!root) return;


    const float buttonWidth = (availableWidth - 8.f) / 3.f;
    const bool  hasChanges = !root->GetPrefabInfo().m_overrides.isEmpty();

    ImGui::BeginDisabled(!hasChanges);
    ImGui::PushStyleColor(ImGuiCol_Button,
        hasChanges ? ImVec4(0.14f, 0.42f, 0.14f, 1.f) : ImVec4(0.15f, 0.15f, 0.15f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.58f, 0.20f, 1.f));

    if (ImGui::Button("Apply", ImVec2(buttonWidth, 0)))
    {
        PrefabManager::applyToPrefab(root);

        Scene* currentScene = app->getModuleScene()->getScene();
        const std::filesystem::path prefabPath = app->getModuleEditor()->getPrefabEditSourcePath();

        std::vector<GameObject*> instances;

        for (GameObject* go : instances)
        {
            Matrix worldMatrix = go->GetTransform()->getGlobalMatrix();

            UID id = go->GetID();
            currentScene->removeGameObject(id);

            GameObject* fresh = PrefabManager::instantiatePrefab(prefabPath, currentScene);
            if (fresh)
            {
                fresh->GetTransform()->setFromGlobalMatrix(worldMatrix);
            }
        }

        app->getModuleEditor()->exitPrefabEdit();
    }

    ImGui::PopStyleColor(2);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(hasChanges ? "Save changes to prefab file" : "No changes to apply");
    }

    ImGui::SameLine(0, 4);

    ImGui::BeginDisabled(!hasChanges);
    if (ImGui::Button("Revert", ImVec2(buttonWidth, 0)))
    {
        PrefabManager::revertToPrefab(root, app->getModuleEditor()->getPrefabEditScene());
        app->getModuleEditor()->setSelectedGameObject(root);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(hasChanges ? "Reload from prefab file" : "No changes to revert");
    }

    ImGui::SameLine(0, 4);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.12f, 0.12f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.17f, 0.17f, 1.f));
    if (ImGui::Button("Exit", ImVec2(buttonWidth, 0)))
    {
        app->getModuleEditor()->exitPrefabEdit();
    }
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Leave without saving  [Esc]");
    }

    ImGui::Spacing();
    ImGui::Separator();
}

void PrefabUI::drawExitOverlay(ImVec2 viewportPos, ImVec2 viewportSize)
{
    if (!app->getModuleEditor()->isInPrefabEditMode()) return;

    constexpr ImGuiWindowFlags OVERLAY_FLAGS =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoSavedSettings;

    constexpr float BUTTON_WIDTH = 170.f;
    ImGui::SetNextWindowPos(
        { viewportPos.x + viewportSize.x - BUTTON_WIDTH - 10.f, viewportPos.y + 28.f },
        ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);

    if (!ImGui::Begin("##pfExit", nullptr, OVERLAY_FLAGS)) { ImGui::End(); return; }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.20f, 1.f));
    // Show the display name (stem), not the full path, in the overlay.
    ImGui::Text("Editing: %s", app->getModuleEditor()->getPrefabEditSourcePath().stem().string().c_str());
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.18f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.38f, 0.07f, 0.07f, 1.f));
    if (ImGui::Button("Exit Prefab Edit  [Esc]", ImVec2(BUTTON_WIDTH, 0)))
    {
        app->getModuleEditor()->exitPrefabEdit();
    }
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Leave without saving. Right-click in Hierarchy to Apply/Revert.");
    }

    ImGui::End();
}

void PrefabUI::drawSavePrefabSection(GameObject* go)
{
    if (!go) return;

    ImGui::Spacing();
    ImGui::SeparatorText("Prefab");

    // The user types a full relative path, e.g. "Assets/Levels/Prop.prefab".
    // No folder is assumed — they own the destination.
    static char pathBuffer[512] = "";
    ImGui::SetNextItemWidth(-80);
    ImGui::InputTextWithHint("##prefabpath", "Assets/.../Name.prefab", pathBuffer, sizeof(pathBuffer));
    ImGui::SameLine();

    if (ImGui::Button("Save") && strlen(pathBuffer) > 0)
    {
        linkAndSavePrefab(go, std::filesystem::path(pathBuffer));
        pathBuffer[0] = '\0';
    }

    if (go->IsPrefabInstance())
    {
        ImGui::Spacing();
        if (ImGui::Button("Edit Prefab"))
        {
            app->getModuleEditor()->enterPrefabEdit(go->GetPrefabInfo().m_sourcePath);
        }
    }
}

void PrefabUI::drawFileDialogInstanceBar(GameObject* go)
{
    if (!go || !go->IsPrefabInstance()) return;

    const PrefabInfo& info = go->GetPrefabInfo();
    const bool        hasOverrides = !info.m_overrides.isEmpty();
    const std::string displayName = info.m_name;

    ImVec2 topLeft = ImGui::GetCursorScreenPos();
    ImVec2 bottomRight = { topLeft.x + ImGui::GetContentRegionAvail().x, topLeft.y + 52.f };
    ImGui::GetWindowDrawList()->AddRectFilled(topLeft, bottomRight, IM_COL32(20, 60, 20, 180));
    ImGui::GetWindowDrawList()->AddRect(topLeft, bottomRight, IM_COL32(50, 160, 50, 140));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.f, 0.35f, 1.f));
    ImGui::Text("  [P]");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);
    ImGui::Text("%s", go->GetName().c_str());
    ImGui::SameLine(0, 4);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
    ImGui::Text("->  %s", displayName.c_str());
    ImGui::PopStyleColor();

    if (hasOverrides)
    {
        ImGui::SameLine(0, 4);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.1f, 1.f));
        ImGui::Text("(overrides)");
        ImGui::PopStyleColor();
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.f);
    constexpr float BUTTON_PAD = 4.f;

    ImGui::BeginDisabled(!hasOverrides);
    ImGui::PushStyleColor(ImGuiCol_Button, hasOverrides ? ImVec4(0.14f, 0.42f, 0.14f, 1.f) : ImVec4(0.15f, 0.15f, 0.15f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.58f, 0.20f, 1.f));
    if (ImGui::SmallButton("Apply"))
    {
        PrefabManager::applyToPrefab(go);
    }
    ImGui::PopStyleColor(2);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(hasOverrides ? "Save changes to prefab file" : "No changes to apply");
    }

    ImGui::SameLine(0, BUTTON_PAD);

    ImGui::BeginDisabled(!hasOverrides);
    if (ImGui::SmallButton("Revert"))
    {
        PrefabManager::revertToPrefab(go, app->getModuleScene()->getScene());
        app->getModuleEditor()->setSelectedGameObject(go);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(hasOverrides ? "Reload from prefab file" : "No changes to revert");
    }

    ImGui::SameLine(0, BUTTON_PAD);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
    if (ImGui::SmallButton("Unlink"))
    {
        go->GetPrefabInfo().clear();
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Break prefab connection");
    }

    ImGui::SameLine(0, BUTTON_PAD);

    if (ImGui::SmallButton("Edit Prefab"))
    {
        app->getModuleEditor()->enterPrefabEdit(info.m_sourcePath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Open prefab editor");
    }

    ImGui::Spacing();
    ImGui::Separator();
}

void PrefabUI::drawNodeContextMenu(GameObject* go, bool prefabMode, bool isEditRoot)
{
    if (!prefabMode) return;

    GameObject* root = app->getModuleEditor()->getPrefabEditRoot();
    if (!root) return;

    const bool hasChanges = !root->GetPrefabInfo().m_overrides.isEmpty();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.75f, 0.2f, 1.f));
    ImGui::Text("Prefab: %s", app->getModuleEditor()->getPrefabEditSourcePath().stem().string().c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::BeginDisabled(!hasChanges);
    ImGui::PushStyleColor(ImGuiCol_Text, hasChanges ? ImVec4(0.3f, 1.f, 0.3f, 1.f) : ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    if (ImGui::MenuItem("Apply  -  Save changes to prefab file"))
    {
        PrefabManager::applyToPrefab(root);
        app->getModuleEditor()->exitPrefabEdit();
    }
    ImGui::PopStyleColor();
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasChanges)
    {
        ImGui::SetTooltip("No changes to apply");
    }

    ImGui::BeginDisabled(!hasChanges);
    if (ImGui::MenuItem("Revert  -  Reload from prefab file"))
    {
        PrefabManager::revertToPrefab(root, app->getModuleEditor()->getPrefabEditScene());
        app->getModuleEditor()->setSelectedGameObject(root);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasChanges)
    {
        ImGui::SetTooltip("No changes to revert");
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
    if (ImGui::MenuItem("Unlink  -  Break prefab connection"))
    {
        root->GetPrefabInfo().clear();
        app->getModuleEditor()->exitPrefabEdit();
    }
    ImGui::PopStyleColor();

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Exit Prefab Edit"))
    {
        app->getModuleEditor()->exitPrefabEdit();
    }
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void PrefabUI::drawPrefabSubMenu(GameObject* go, Scene* scene)
{
    if (!go || !ImGui::BeginMenu("Prefab")) return;

    // User types a full relative save path — no folder is assumed.
    static char pathBuffer[512] = "";
    ImGui::SetNextItemWidth(200.f);
    ImGui::InputTextWithHint("##pfpath", "Assets/.../Name.prefab", pathBuffer, sizeof(pathBuffer));
    ImGui::SameLine();

    if (ImGui::Button("Save"))
    {
        const std::filesystem::path savePath =
            strlen(pathBuffer) > 0
            ? std::filesystem::path(pathBuffer)
            : std::filesystem::path("Assets") / (go->GetName() + ".prefab");

        linkAndSavePrefab(go, savePath);
        memset(pathBuffer, 0, sizeof(pathBuffer));
    }

    if (go->IsPrefabInstance())
    {
        ImGui::Separator();
        const PrefabInfo& info = go->GetPrefabInfo();

        if (ImGui::MenuItem("Edit Prefab"))
        {
            app->getModuleEditor()->enterPrefabEdit(info.m_sourcePath);
        }
        if (ImGui::MenuItem("Apply to Prefab"))
        {
            PrefabManager::applyToPrefab(go);
        }
        if (ImGui::MenuItem("Revert to Prefab"))
        {
            PrefabManager::revertToPrefab(go, scene);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Unlink"))
        {
            go->GetPrefabInfo().clear();
        }
    }

    ImGui::EndMenu();
}

void PrefabUI::markTransformOverride(GameObject* go)
{
    if (!go) return;

    const int transformType = static_cast<int>(ComponentType::TRANSFORM);

    GameObject* current = go;
    while (current)
    {
        if (current->IsPrefabInstance())
        {
            auto& props = current->GetPrefabInfo().m_overrides.m_modifiedProperties[transformType];
            props.insert("position");
            props.insert("rotation");
            props.insert("scale");
            return;
        }
        Transform* parentTransform = current->GetTransform()->getRoot();
        current = parentTransform ? parentTransform->getOwner() : nullptr;
    }
}

// ---------------------------------------------------------------------------
// File-dialog context menu
// sourcePath — the full path of the .prefab file being right-clicked.
// buffers    — must be allocated by the caller with at least 512 bytes each.
// ---------------------------------------------------------------------------
void PrefabUI::drawFileDialogItemContextMenu(const std::filesystem::path& sourcePath,
    bool& outShowVariantModal,
    bool& outRenamingPrefab,
    FileDialogBuffers& buffers)
{
    if (!ImGui::BeginPopupContextItem("ItemContext")) return;

    const std::string displayName = sourcePath.stem().string();
    const std::string pathStr = sourcePath.string();
    const std::filesystem::path realPath = sourcePath.parent_path() / sourcePath.stem();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.f, 0.35f, 1.f));
    ImGui::Text("[P]  %s", displayName.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (ImGui::MenuItem("Add to Scene"))
    {
        Scene* scene = app->getModuleScene()->getScene();
        if (scene)
        {

            GameObject* go = PrefabManager::instantiatePrefab(realPath, scene);
            if (go) 
            {
                app->getModuleEditor()->setSelectedGameObject(go);
            }
        }
    }

    if (ImGui::MenuItem("Edit Prefab..."))
    {
        app->getModuleEditor()->enterPrefabEdit(realPath);
    }

    ImGui::Separator();

    GameObject* selected = app->getModuleEditor()->getSelectedGameObject();
    const bool  isLinked = selected && selected->IsPrefabInstance() && selected->GetPrefabInfo().m_sourcePath == sourcePath;
    const bool  hasOverrides = isLinked && !selected->GetPrefabInfo().m_overrides.isEmpty();

    ImGui::BeginDisabled(!hasOverrides);
    if (ImGui::MenuItem("Apply to Prefab"))
    {
        PrefabManager::applyToPrefab(selected);
    }
    if (ImGui::MenuItem("Revert to Prefab"))
    {
        PrefabManager::revertToPrefab(selected, app->getModuleScene()->getScene());
        app->getModuleEditor()->setSelectedGameObject(selected);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasOverrides)
    {
        ImGui::SetTooltip("Select a linked instance first");
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Create Variant..."))
    {
        outShowVariantModal = true;
        strncpy_s(buffers.variantSource, buffers.variantSourceSize, pathStr.c_str(), buffers.variantSourceSize - 1);

        // Suggest a sibling file: same directory, stem + "_variant".
        const std::string variantPath = (sourcePath.parent_path() / (displayName + "_variant.prefab")).string();
        strncpy_s(buffers.variantDest, buffers.variantDestSize, variantPath.c_str(), buffers.variantDestSize - 1);
    }

    if (ImGui::MenuItem("Rename..."))
    {
        outRenamingPrefab = true;
        strncpy_s(buffers.renameSource, buffers.renameSourceSize, pathStr.c_str(), buffers.renameSourceSize - 1);
        strncpy_s(buffers.renameDest, buffers.renameDestSize, pathStr.c_str(), buffers.renameDestSize - 1);
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Delete Prefab File"))
    {
        // Use the source path directly — no "Assets/Prefabs/" assumption.
        if (FileIO::exists(sourcePath))
        {
            FileIO::remove(sourcePath);
            app->getModuleAssets()->refresh();
        }
    }
    ImGui::PopStyleColor();

    ImGui::EndPopup();
}

void PrefabUI::drawFileDialogModals(bool& showVariantModal,
    bool& showSavePrefabModal,
    bool& renamingPrefab,
    FileDialogBuffers& buffers)
{
    const ImVec2 screenCenter = ImGui::GetMainViewport()->GetCenter();

    if (showVariantModal) { ImGui::OpenPopup("##pfVariantModal"); showVariantModal = false; }
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 420, 0 });
    if (ImGui::BeginPopupModal("##pfVariantModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Create variant of:");
        ImGui::TextDisabled("%s", buffers.variantSource);
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("Destination path##vname", buffers.variantDest, buffers.variantDestSize);
        ImGui::Spacing();
        if (ImGui::Button("Create", { 100, 0 }))
        {
            if (strlen(buffers.variantDest) > 0)
            {
                // Both buffers hold full paths.
                PrefabManager::createVariant(
                    std::filesystem::path(buffers.variantSource),
                    std::filesystem::path(buffers.variantDest));
                app->getModuleAssets()->refresh();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 80, 0 }))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ── Rename modal ─────────────────────────────────────────────────────────
    if (renamingPrefab) { ImGui::OpenPopup("##pfRenameModal"); renamingPrefab = false; }
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 420, 0 });
    if (ImGui::BeginPopupModal("##pfRenameModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Rename prefab:");
        ImGui::TextDisabled("%s", buffers.renameSource);
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        const bool enterPressed = ImGui::InputText("New path##rname", buffers.renameDest, buffers.renameDestSize, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if ((ImGui::Button("Rename", { 100, 0 }) || enterPressed)
            && strlen(buffers.renameDest) > 0
            && strcmp(buffers.renameSource, buffers.renameDest) != 0)
        {
            // Both buffers hold full paths — no folder is assumed.
            if (FileIO::move(
                std::filesystem::path(buffers.renameSource),
                std::filesystem::path(buffers.renameDest)))
            {
                app->getModuleAssets()->refresh();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 80, 0 }))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (showSavePrefabModal) 
    { 
        ImGui::OpenPopup("##pfSaveModal"); showSavePrefabModal = false; 
    }

    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 420, 0 });

    if (ImGui::BeginPopupModal("##pfSaveModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        GameObject* selected = app->getModuleEditor()->getSelectedGameObject();
        ImGui::Text("Save \"%s\" as prefab:", selected ? selected->GetName().c_str() : "?");
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        ImGui::TextDisabled("Full relative path, e.g. Assets/Prefabs/Hero.prefab");
        const bool enterPressed = ImGui::InputText("##pfn", buffers.savePrefab, buffers.savePrefabSize, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if ((ImGui::Button("Save", { 100, 0 }) || enterPressed)
            && strlen(buffers.savePrefab) > 0)
        {
            if (selected)
            {
                linkAndSavePrefab(selected, std::filesystem::path(buffers.savePrefab));
                app->getModuleAssets()->refresh();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 80, 0 }))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}