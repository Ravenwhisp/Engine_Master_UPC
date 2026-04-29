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

#include "PrefabAsset.h"
#include "PrefabEditSession.h"
#include <FileIO.h>

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
        app->getModuleAssets()->applyPrefab(root);
        app->getModuleAssets()->refresh();
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
        auto prefab = app->getModuleAssets()->load<PrefabAsset>(makeRef(root->GetPrefabInfo().m_assetUID));
        prefab->revert(root);
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
    // Show the display name (stem), not the full path, in the overlay.    ImGui::PopStyleColor();

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

    if (go->IsPrefabInstance())
    {
        ImGui::Spacing();
        if (ImGui::Button("Edit Prefab"))
        {
            app->getModuleEditor()->enterPrefabEdit(go->GetPrefabInfo().m_assetUID);
        }
    }
}

void PrefabUI::drawFileDialogInstanceBar(GameObject* go)
{
    if (!go || !go->IsPrefabInstance()) return;

    const PrefabInfo& info = go->GetPrefabInfo();
    const bool        hasOverrides = !info.m_overrides.isEmpty();

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
        app->getModuleAssets()->applyPrefab(go);
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
        auto prefab = app->getModuleAssets()->load<PrefabAsset>(makeRef(go->GetPrefabInfo().m_assetUID));
        prefab->revert(go);
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
        app->getModuleEditor()->enterPrefabEdit(info.m_assetUID);
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

    const bool hasChanges = !root->GetPrefabInfo().m_overrides.isEmpty();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.75f, 0.2f, 1.f));
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::BeginDisabled(!hasChanges);
    ImGui::PushStyleColor(ImGuiCol_Text, hasChanges ? ImVec4(0.3f, 1.f, 0.3f, 1.f) : ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    if (ImGui::MenuItem("Apply  -  Save changes to prefab file"))
    {
        app->getModuleAssets()->applyPrefab(root);
        app->getModuleAssets()->refresh();
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
        auto prefab = app->getModuleAssets()->load<PrefabAsset>(makeRef(root->GetPrefabInfo().m_assetUID));
        prefab->revert(root);
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

    if (ImGui::Button("Save"))
    {
        app->getModuleAssets()->applyPrefab(go);
    }

    if (go->IsPrefabInstance())
    {
        ImGui::Separator();
        const PrefabInfo& info = go->GetPrefabInfo();

        if (ImGui::MenuItem("Edit Prefab"))
        {
            app->getModuleEditor()->enterPrefabEdit(info.m_assetUID);
        }
        if (ImGui::MenuItem("Apply to Prefab"))
        {
            app->getModuleAssets()->applyPrefab(go);
        }
        if (ImGui::MenuItem("Revert to Prefab"))
        {
            auto prefab = app->getModuleAssets()->load<PrefabAsset>(makeRef(go->GetPrefabInfo().m_assetUID));
            prefab->revert(go);
            app->getModuleEditor()->setSelectedGameObject(go);
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


void PrefabUI::drawFileDialogItemContextMenu(const std::filesystem::path& sourcePath,
    bool& outShowVariantModal,
    bool& outRenamingPrefab)
{
    if (!ImGui::BeginPopupContextItem("ItemContext")) return;

    const std::string displayName = sourcePath.stem().string();
    const std::string pathStr = sourcePath.string();

    // Resolve the asset UID once — all asset operations go through this.
    // File operations (delete, rename, variant) still use sourcePath directly.
    const UID itemUID = app->getModuleAssets()->findUID(sourcePath);
    const bool assetRegistered = isValidUID(itemUID);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.f, 0.35f, 1.f));
    ImGui::Text("[P]  %s", displayName.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::BeginDisabled(!assetRegistered);
    if (ImGui::MenuItem("Add to Scene"))
    {
        Scene* scene = app->getModuleScene()->getScene();
        if (scene)
        {
            // UID-based spawn: no path arithmetic, no extension stripping.
            auto prefab = app->getModuleAssets()->load<PrefabAsset>(makeRef(itemUID));
            std::unique_ptr<GameObject> gameObject = prefab->spawnPrefab();
            scene->addGameObject(std::move(gameObject));
            if (gameObject)
            {
                app->getModuleEditor()->setSelectedGameObject(gameObject.get());
            }
        }
    }

    if (ImGui::MenuItem("Edit Prefab..."))
    {
        // UID-based enter: no path overload needed.
        app->getModuleEditor()->enterPrefabEdit(itemUID);
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    GameObject* selected = app->getModuleEditor()->getSelectedGameObject();
    // isLinked: the selected instance points at the same registered asset.
    // Replaces the removed PrefabInfo::m_sourcePath comparison.
    const bool  isLinked = selected
        && selected->IsPrefabInstance()
        && selected->GetPrefabInfo().m_assetUID == itemUID;
    const bool  hasOverrides = isLinked && !selected->GetPrefabInfo().m_overrides.isEmpty();

    ImGui::BeginDisabled(!hasOverrides);
    if (ImGui::MenuItem("Apply to Prefab"))
    {
        app->getModuleAssets()->applyPrefab(selected);
    }
    if (ImGui::MenuItem("Revert to Prefab"))
    {
        auto prefab = app->getModuleAssets()->load<PrefabAsset>(makeRef(selected->GetPrefabInfo().m_assetUID));
        prefab->revert(selected);
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

    }

    if (ImGui::MenuItem("Rename..."))
    {
        outRenamingPrefab = true;
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Delete Prefab File"))
    {
        if (FileIO::exists(sourcePath))
        {
            FileIO::remove(sourcePath);
            app->getModuleAssets()->refresh();
        }
    }
    ImGui::PopStyleColor();

    ImGui::EndPopup();
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