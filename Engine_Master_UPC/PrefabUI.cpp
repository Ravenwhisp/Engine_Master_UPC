#include "Globals.h"
#include "PrefabUI.h"

#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleAssets.h"
#include "ModuleScene.h"
#include "PrefabManager.h"

#include "Scene.h"
#include "GameObject.h"
#include "PrefabInstanceComponent.h"
#include "ComponentType.h"
#include "Transform.h"

#include "Prefab.h"
#include "PrefabEditSession.h"
#include "AssetsDictionary.h"
#include <FileIO.h>

static void linkAndSavePrefab(GameObject* go, const std::filesystem::path& savePath)
{
    AssetReference ref;
    Prefab tempPrefab(ref);
    tempPrefab.setUID(GenerateUID());
    tempPrefab.buildFrom(go);
    tempPrefab.m_sourcePath = savePath;

    if (!app->getModuleAssets()->save(tempPrefab, savePath))
        return;

    auto* preComp = static_cast<PrefabInstanceComponent*>(
        go->AddComponentWithUID(ComponentType::PREFAB_INSTANCE, GenerateUID()));
    if (preComp)
    {
        preComp->getData().m_sourcePath = savePath;
        const UID assetUID = app->getModuleAssets()->getIndex().findUID(savePath);
        if (isValidUID(assetUID))
            preComp->getData().m_assetUID = assetUID;
    }

    const UID existingUID = app->getModuleAssets()->getIndex().findUID(savePath);
    if (isValidUID(existingUID))
        app->getModuleAssets()->unload(AssetReference(existingUID));
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

bool PrefabUI::hasPrefabOverrides(GameObject* go)
{
    if (!go) return false;
    auto* pc = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    return pc && pc->isInstance() && !pc->getData().m_overrides.isEmpty();
}

void PrefabUI::drawApplyRevertButtons(GameObject* go, Scene* scene, bool useSmallButton,
                                      bool& outApply, bool& outRevert)
{
    outApply = false;
    outRevert = false;
    const bool hasChanges = hasPrefabOverrides(go);

    ImGui::BeginDisabled(!hasChanges);
    if (useSmallButton)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, hasChanges ? ImVec4(0.14f, 0.42f, 0.14f, 1.f) : ImVec4(0.15f, 0.15f, 0.15f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.58f, 0.20f, 1.f));
        if (ImGui::SmallButton("Apply"))
            outApply = true;
        ImGui::PopStyleColor(2);
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip(hasChanges ? "Save changes to prefab file" : "No changes to apply");

        ImGui::SameLine(0, 4);

        ImGui::BeginDisabled(!hasChanges);
        if (ImGui::SmallButton("Revert"))
            outRevert = true;
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip(hasChanges ? "Reload from prefab file" : "No changes to revert");
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, hasChanges ? ImVec4(0.3f, 1.f, 0.3f, 1.f) : ImVec4(0.5f, 0.5f, 0.5f, 1.f));
        if (ImGui::MenuItem("Apply  -  Save changes to prefab file"))
            outApply = true;
        ImGui::PopStyleColor();
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasChanges)
            ImGui::SetTooltip("No changes to apply");

        ImGui::BeginDisabled(!hasChanges);
        if (ImGui::MenuItem("Revert  -  Reload from prefab file"))
            outRevert = true;
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasChanges)
            ImGui::SetTooltip("No changes to revert");
    }

    if (outApply)
        app->getModuleAssets()->getPrefabManager()->applyPrefab(go);
    if (outRevert)
    {
        app->getModuleAssets()->getPrefabManager()->revertPrefab(go, scene);
        app->getModuleEditor()->setSelectedGameObject(go);
    }
}

void PrefabUI::drawApplyRevertBar(float availableWidth)
{
    GameObject* root = app->getModuleEditor()->getPrefabEditRoot();
    if (!root) return;

    const float buttonWidth = (availableWidth - 8.f) / 3.f;
    const bool hasChanges = app->getModuleEditor()->isInPrefabEditMode() || hasPrefabOverrides(root);

    ImGui::BeginDisabled(!hasChanges);
    ImGui::PushStyleColor(ImGuiCol_Button,
        hasChanges ? ImVec4(0.14f, 0.42f, 0.14f, 1.f) : ImVec4(0.15f, 0.15f, 0.15f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.58f, 0.20f, 1.f));

    if (ImGui::Button("Apply", ImVec2(buttonWidth, 0)))
    {
        app->getModuleAssets()->getPrefabManager()->applyPrefab(root);
        app->getModuleEditor()->exitPrefabEdit();
    }

    ImGui::PopStyleColor(2);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(hasChanges ? "Save changes to prefab file" : "No changes to apply");

    ImGui::SameLine(0, 4);

    ImGui::BeginDisabled(!hasChanges);
    if (ImGui::Button("Revert", ImVec2(buttonWidth, 0)))
    {
        app->getModuleAssets()->getPrefabManager()->revertPrefab(root, app->getModuleEditor()->getPrefabEditScene());
        app->getModuleEditor()->setSelectedGameObject(root);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(hasChanges ? "Reload from prefab file" : "No changes to revert");

    ImGui::SameLine(0, 4);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.12f, 0.12f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.17f, 0.17f, 1.f));
    if (ImGui::Button("Exit", ImVec2(buttonWidth, 0)))
    {
        app->getModuleEditor()->exitPrefabEdit();
    }
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Leave without saving  [Esc]");

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

    auto* _preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (_preComp && _preComp->isInstance())
    {
        ImGui::Spacing();
        if (ImGui::Button("Edit Prefab"))
        {
            const PrefabInstanceInfo* info = &_preComp->getData();
            if (info) app->getModuleEditor()->enterPrefabEdit(info->m_sourcePath);
        }
    }
}

void PrefabUI::drawFileDialogInstanceBar(GameObject* go)
{
    if (!go) return;
    auto* _preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (!_preComp || !_preComp->isInstance()) return;

    const PrefabInstanceInfo* info = &_preComp->getData();
    const bool hasOverrides = info && !info->m_overrides.isEmpty();

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

    if (hasOverrides)
    {
        ImGui::SameLine(0, 4);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.1f, 1.f));
        ImGui::Text("(overrides)");
        ImGui::PopStyleColor();
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.f);
    constexpr float BUTTON_PAD = 4.f;

    bool applied = false, reverted = false;
    drawApplyRevertButtons(go, app->getModuleScene()->getScene(), true, applied, reverted);

    ImGui::SameLine(0, BUTTON_PAD);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
    if (ImGui::SmallButton("Unlink"))
    {
        { auto* _pc = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE); if (_pc) _pc->getData().clear(); }
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Break prefab connection");
    }

    ImGui::SameLine(0, BUTTON_PAD);

    if (ImGui::SmallButton("Edit Prefab"))
    {
        if (info) app->getModuleEditor()->enterPrefabEdit(info->m_sourcePath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Open prefab editor");
    }

    ImGui::Spacing();
    ImGui::Separator();
}

void PrefabUI::drawNodeContextMenu(bool prefabMode)
{
    if (!prefabMode) return;

    GameObject* root = app->getModuleEditor()->getPrefabEditRoot();
    if (!root) return;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.75f, 0.2f, 1.f));
    ImGui::Text("Prefab: %s", app->getModuleEditor()->getPrefabEditSourcePath().stem().string().c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    bool applied = false, reverted = false;
    drawApplyRevertButtons(root, app->getModuleScene()->getScene(), false, applied, reverted);
    if (applied)
        app->getModuleEditor()->exitPrefabEdit();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
    if (ImGui::MenuItem("Unlink  -  Break prefab connection"))
    {
        { auto* _pc = root->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE); if (_pc) _pc->getData().clear(); }
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

    auto* _preComp = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
    if (_preComp && _preComp->isInstance())
    {
        ImGui::Separator();
        const PrefabInstanceInfo* info = &_preComp->getData();

        if (ImGui::MenuItem("Edit Prefab"))
        {
            if (info) app->getModuleEditor()->enterPrefabEdit(info->m_sourcePath);
        }

        bool applied = false, reverted = false;
        drawApplyRevertButtons(go, scene, false, applied, reverted);

        ImGui::Separator();
        if (ImGui::MenuItem("Unlink"))
        {
            { auto* _pc = go->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE); if (_pc) _pc->getData().clear(); }
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
        auto* preComp = current->GetComponentAs<PrefabInstanceComponent>(ComponentType::PREFAB_INSTANCE);
        if (preComp && preComp->isInstance())
        {
            auto& props = preComp->getData().m_overrides.m_modifiedProperties[transformType];
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

            GameObject* go = app->getModuleAssets()->getPrefabManager()->spawnPrefab(realPath, scene);
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
    bool applied = false, reverted = false;
    if (selected)
        drawApplyRevertButtons(selected, app->getModuleScene()->getScene(), false, applied, reverted);

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
        strncpy_s(buffers.renameSource, buffers.renameSourceSize, realPath.string().c_str(), buffers.renameSourceSize - 1);
        strncpy_s(buffers.renameDest, buffers.renameDestSize, realPath.stem().string().c_str(), buffers.renameDestSize - 1);
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Delete Prefab File"))
    {
        // Use the source path directly — no "Assets/Prefabs/" assumption.
        if (FileIO::exists(sourcePath))
        {
            FileIO::remove(sourcePath);
            app->getModuleAssets()->unregisterAsset(sourcePath.parent_path() / sourcePath.stem());
        }
    }
    ImGui::PopStyleColor();

    ImGui::EndPopup();
}

static bool beginModal(const char* id, bool& triggerFlag, const ImVec2& screenCenter)
{
    if (triggerFlag) { ImGui::OpenPopup(id); triggerFlag = false; }
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 420, 0 });
    return ImGui::BeginPopupModal(id, nullptr, ImGuiWindowFlags_AlwaysAutoResize);
}

void PrefabUI::drawFileDialogModals(bool& showVariantModal,
    bool& showSavePrefabModal,
    bool& renamingPrefab,
    bool& renamingAsset,
    FileDialogBuffers& buffers)
{
    const ImVec2 screenCenter = ImGui::GetMainViewport()->GetCenter();

    if (beginModal("##pfVariantModal", showVariantModal, screenCenter))
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
                app->getModuleAssets()->getPrefabManager()->createVariant(
                    std::filesystem::path(buffers.variantSource),
                    std::filesystem::path(buffers.variantDest));
                AssetReference ref;
                app->getModuleAssets()->importAsset(std::filesystem::path(buffers.variantDest), ref);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 80, 0 })) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (beginModal("##pfRenameModal", renamingPrefab, screenCenter))
    {
        ImGui::Text("Rename prefab:");
        ImGui::Text("From:  %s", buffers.renameSource);
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        const bool enterPressed = ImGui::InputText("New name##rname", buffers.renameDest, buffers.renameDestSize, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if ((ImGui::Button("Rename", { 100, 0 }) || enterPressed)
            && strlen(buffers.renameDest) > 0)
        {
            const auto srcAsset = std::filesystem::path(buffers.renameSource);
            if (strcmp(buffers.renameDest, srcAsset.stem().string().c_str()) != 0)
            {
                const auto dir = srcAsset.parent_path();
                const auto ext = srcAsset.extension().string();
                const auto dstAsset = dir / (std::string(buffers.renameDest) + ext);
                auto srcMeta = srcAsset; srcMeta += ".metadata";
                auto dstMeta = dstAsset; dstMeta += ".metadata";

                app->getModuleAssets()->unregisterAsset(srcAsset);
                bool ok = FileIO::move(srcAsset, dstAsset);
                if (ok) ok = FileIO::move(srcMeta, dstMeta);
                if (ok)
                {
                    AssetReference ref;
                    app->getModuleAssets()->importAsset(dstAsset, ref);
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 80, 0 })) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (beginModal("##pfSaveModal", showSavePrefabModal, screenCenter))
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
                linkAndSavePrefab(selected, std::filesystem::path(buffers.savePrefab));
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 80, 0 })) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

std::vector<PrefabUI::PrefabFileInfo> PrefabUI::listPrefabsInfo(const std::filesystem::path& searchRoot)
{
    std::vector<PrefabFileInfo> results;
    if (!std::filesystem::exists(searchRoot)) return results;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchRoot))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".prefab") continue;

        PrefabFileInfo info;
        info.m_sourcePath = entry.path();
        info.m_name = entry.path().stem().string();
        results.push_back(std::move(info));
    }

    return results;
}