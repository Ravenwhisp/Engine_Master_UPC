#include "Globals.h"
#include "PrefabUI.h"

#include "Application.h"
#include "ComponentType.h"
#include "EditorModule.h"
#include "FileSystemModule.h"
#include "GameObject.h"
#include "PrefabEditSession.h"
#include "PrefabManager.h"
#include "SceneModule.h"

static void linkAndSavePrefab(GameObject* go, const char* prefabName)
{
    if (!PrefabManager::createPrefab(go, prefabName))
    {
        return;
    }

    PrefabInstanceData instanceData;
    instanceData.m_prefabName = prefabName;
    instanceData.m_prefabUID = PrefabManager::makePrefabUID(prefabName);
    PrefabManager::linkInstance(go, instanceData);
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
    PrefabEditSession* session = app->getEditorModule()->getPrefabSession();
    if (!session || !session->m_active || !session->m_rootObject)
    {
        return;
    }

    const float               buttonWidth = (availableWidth - 8.f) / 3.f;
    const PrefabInstanceData* instance = PrefabManager::getInstanceData(session->m_rootObject);
    const bool                hasChanges = instance && !instance->m_overrides.isEmpty();

    ImGui::BeginDisabled(!hasChanges);
    ImGui::PushStyleColor(ImGuiCol_Button,
        hasChanges ? ImVec4(0.14f, 0.42f, 0.14f, 1.f) : ImVec4(0.15f, 0.15f, 0.15f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.58f, 0.20f, 1.f));

    if (ImGui::Button("Apply", ImVec2(buttonWidth, 0)))
    {
        PrefabManager::applyToPrefab(session->m_rootObject);
        app->getEditorModule()->exitPrefabEdit();
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
        PrefabManager::revertToPrefab(session->m_rootObject, session->m_isolatedScene.get());
        app->getEditorModule()->setSelectedGameObject(session->m_rootObject);
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
        app->getEditorModule()->exitPrefabEdit();
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
    PrefabEditSession* session = app->getEditorModule()->getPrefabSession();
    if (!session || !session->m_active)
    {
        return;
    }

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

    if (!ImGui::Begin("##pfExit", nullptr, OVERLAY_FLAGS))
    {
        ImGui::End();
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.20f, 1.f));
    ImGui::Text("Editing: %s", session->m_prefabName.c_str());
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.18f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.38f, 0.07f, 0.07f, 1.f));
    if (ImGui::Button("Exit Prefab Edit  [Esc]", ImVec2(BUTTON_WIDTH, 0)))
    {
        app->getEditorModule()->exitPrefabEdit();
    }
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Leave without saving. Right-click in Hierarchy to Apply/Revert.");
    }

    ImGui::End();
}

void PrefabUI::drawInstanceBadge(const GameObject* go)
{
    if (!PrefabManager::isPrefabInstance(go))
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.75f, 1.0f, 1.f));
    ImGui::Text("[Prefab: %s]", PrefabManager::getPrefabName(go).c_str());
    ImGui::PopStyleColor();

    const PrefabInstanceData* instance = PrefabManager::getInstanceData(go);
    if (instance && !instance->m_overrides.isEmpty())
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
    if (!go)
    {
        return;
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Prefab");

    static char nameBuffer[256] = "";
    ImGui::SetNextItemWidth(-80);
    ImGui::InputText("##prefabname", nameBuffer, sizeof(nameBuffer));
    ImGui::SameLine();

    if (ImGui::Button("Save") && strlen(nameBuffer) > 0)
    {
        linkAndSavePrefab(go, nameBuffer);
        nameBuffer[0] = '\0';
    }

    if (PrefabManager::isPrefabInstance(go))
    {
        ImGui::Spacing();
        if (ImGui::Button("Edit Prefab"))
        {
            app->getEditorModule()->enterPrefabEdit(PrefabManager::getPrefabName(go));
        }
    }
}

void PrefabUI::drawFileDialogInstanceBar(GameObject* go)
{
    if (!go || !PrefabManager::isPrefabInstance(go))
    {
        return;
    }

    const std::string         prefabName = PrefabManager::getPrefabName(go);
    const PrefabInstanceData* instance = PrefabManager::getInstanceData(go);
    const bool                hasOverrides = instance && !instance->m_overrides.isEmpty();

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
    ImGui::Text("->  %s", prefabName.c_str());
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
    ImGui::PushStyleColor(ImGuiCol_Button,
        hasOverrides ? ImVec4(0.14f, 0.42f, 0.14f, 1.f) : ImVec4(0.15f, 0.15f, 0.15f, 1.f));
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
        PrefabManager::revertToPrefab(go, app->getSceneModule());
        app->getEditorModule()->setSelectedGameObject(go);
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
        PrefabManager::unlinkInstance(go);
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Break prefab connection");
    }

    ImGui::SameLine(0, BUTTON_PAD);

    if (ImGui::SmallButton("Edit Prefab"))
    {
        app->getEditorModule()->enterPrefabEdit(prefabName);
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
    if (!prefabMode)
    {
        return;
    }

    PrefabEditSession* session = app->getEditorModule()->getPrefabSession();
    const PrefabInstanceData* instance = PrefabManager::getInstanceData(session->m_rootObject);
    const bool                hasChanges = instance && !instance->m_overrides.isEmpty();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.75f, 0.2f, 1.f));
    ImGui::Text("Prefab: %s", session->m_prefabName.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::BeginDisabled(!hasChanges);
    ImGui::PushStyleColor(ImGuiCol_Text, hasChanges ? ImVec4(0.3f, 1.f, 0.3f, 1.f) : ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    if (ImGui::MenuItem("Apply  -  Save changes to prefab file"))
    {
        PrefabManager::applyToPrefab(session->m_rootObject);
        app->getEditorModule()->exitPrefabEdit();
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
        PrefabManager::revertToPrefab(session->m_rootObject, session->m_isolatedScene.get());
        app->getEditorModule()->setSelectedGameObject(session->m_rootObject);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasChanges)
    {
        ImGui::SetTooltip("No changes to revert");
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
    if (ImGui::MenuItem("Unlink  -  Break prefab connection"))
    {
        PrefabManager::unlinkInstance(session->m_rootObject);
        app->getEditorModule()->exitPrefabEdit();
    }
    ImGui::PopStyleColor();

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Exit Prefab Edit"))
    {
        app->getEditorModule()->exitPrefabEdit();
    }
    ImGui::PopStyleColor();
    ImGui::Separator();
}

void PrefabUI::drawPrefabSubMenu(GameObject* go, SceneModule* scene)
{
    if (!go || !ImGui::BeginMenu("Prefab"))
    {
        return;
    }

    static char nameBuffer[128] = "";
    ImGui::SetNextItemWidth(140.f);
    ImGui::InputTextWithHint("##pfn", go->GetName().c_str(), nameBuffer, sizeof(nameBuffer));
    ImGui::SameLine();

    if (ImGui::Button("Save"))
    {
        std::string name = strlen(nameBuffer) > 0 ? nameBuffer : go->GetName();
        linkAndSavePrefab(go, name.c_str());
        memset(nameBuffer, 0, sizeof(nameBuffer));
    }

    if (PrefabManager::isPrefabInstance(go))
    {
        ImGui::Separator();
        if (ImGui::MenuItem("Edit Prefab"))
        {
            app->getEditorModule()->enterPrefabEdit(PrefabManager::getPrefabName(go));
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
            PrefabManager::unlinkInstance(go);
        }
    }

    ImGui::EndMenu();
}


void PrefabUI::markTransformOverride(GameObject* go)
{
    if (!go)
    {
        return;
    }

    const int transformType = static_cast<int>(ComponentType::TRANSFORM);

    GameObject* current = go;
    while (current)
    {
        if (PrefabManager::isPrefabInstance(current))
        {
            PrefabManager::markPropertyOverride(current, transformType, "position");
            PrefabManager::markPropertyOverride(current, transformType, "rotation");
            PrefabManager::markPropertyOverride(current, transformType, "scale");
            return;
        }
        Transform* parentTransform = current->GetTransform()->getRoot();
        current = parentTransform ? parentTransform->getOwner() : nullptr;
    }
}


void PrefabUI::drawFileDialogItemContextMenu(const std::string& prefabName,
    bool& outShowVariantModal,
    bool& outRenamingPrefab,
    FileDialogBuffers& buffers)
{
    if (!ImGui::BeginPopupContextItem("ItemContext"))
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.f, 0.35f, 1.f));
    ImGui::Text("[P]  %s", prefabName.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (ImGui::MenuItem("Add to Scene"))
    {
        SceneModule* scene = app->getSceneModule();
        if (scene)
        {
            GameObject* gameObject = PrefabManager::instantiatePrefab(prefabName, scene);
            if (gameObject)
            {
                app->getEditorModule()->setSelectedGameObject(gameObject);
            }
        }
    }

    if (ImGui::MenuItem("Edit Prefab..."))
    {
        app->getEditorModule()->enterPrefabEdit(prefabName);
    }

    ImGui::Separator();

    GameObject* selected = app->getEditorModule()->getSelectedGameObject();
    const bool                isLinked = selected
        && PrefabManager::isPrefabInstance(selected)
        && PrefabManager::getPrefabName(selected) == prefabName;
    const PrefabInstanceData* instance = isLinked ? PrefabManager::getInstanceData(selected) : nullptr;
    const bool                hasOverrides = instance && !instance->m_overrides.isEmpty();

    ImGui::BeginDisabled(!hasOverrides);
    if (ImGui::MenuItem("Apply to Prefab"))
    {
        PrefabManager::applyToPrefab(selected);
    }
    if (ImGui::MenuItem("Revert to Prefab"))
    {
        PrefabManager::revertToPrefab(selected, app->getSceneModule());
        app->getEditorModule()->setSelectedGameObject(selected);
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
        strncpy_s(buffers.variantSource, buffers.variantSourceSize, prefabName.c_str(), buffers.variantSourceSize - 1);
        snprintf(buffers.variantDest, buffers.variantDestSize, "%s_variant", prefabName.c_str());
    }

    if (ImGui::MenuItem("Rename..."))
    {
        outRenamingPrefab = true;
        strncpy_s(buffers.renameSource, buffers.renameSourceSize, prefabName.c_str(), buffers.renameSourceSize - 1);
        strncpy_s(buffers.renameDest, buffers.renameDestSize, prefabName.c_str(), buffers.renameDestSize - 1);
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.3f, 0.3f, 1.f));
    if (ImGui::MenuItem("Delete Prefab File"))
    {
        const std::string path = std::string("Assets/Prefabs/") + prefabName + ".prefab";
        if (app->getFileSystemModule()->exists(path.c_str()))
        {
            app->getFileSystemModule()->deleteFile(path.c_str());
            app->getFileSystemModule()->rebuild();
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

    if (showVariantModal)
    {
        ImGui::OpenPopup("##pfVariantModal");
        showVariantModal = false;
    }
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 320, 0 });
    if (ImGui::BeginPopupModal("##pfVariantModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Create variant of:  \"%s\"", buffers.variantSource);
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("Variant name##vname", buffers.variantDest, buffers.variantDestSize);
        ImGui::Spacing();
        if (ImGui::Button("Create", { 100, 0 }))
        {
            if (strlen(buffers.variantDest) > 0 && PrefabManager::createVariant(buffers.variantSource, buffers.variantDest))
            {
                app->getFileSystemModule()->rebuild();
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

    if (renamingPrefab)
    {
        ImGui::OpenPopup("##pfRenameModal");
        renamingPrefab = false;
    }
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 300, 0 });
    if (ImGui::BeginPopupModal("##pfRenameModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Rename prefab:  \"%s\"", buffers.renameSource);
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        const bool enterPressed = ImGui::InputText("New name##rname", buffers.renameDest, buffers.renameDestSize,
            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if ((ImGui::Button("Rename", { 100, 0 }) || enterPressed)
            && strlen(buffers.renameDest) > 0
            && strcmp(buffers.renameSource, buffers.renameDest) != 0)
        {
            const std::string oldPath = std::string("Assets/Prefabs/") + buffers.renameSource + ".prefab";
            const std::string newPath = std::string("Assets/Prefabs/") + buffers.renameDest + ".prefab";
            if (app->getFileSystemModule()->move(oldPath.c_str(), newPath.c_str()))
            {
                app->getFileSystemModule()->rebuild();
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
        ImGui::OpenPopup("##pfSaveModal");
        showSavePrefabModal = false;
    }
    ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 300, 0 });
    if (ImGui::BeginPopupModal("##pfSaveModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        GameObject* selected = app->getEditorModule()->getSelectedGameObject();
        ImGui::Text("Save \"%s\" as prefab:", selected ? selected->GetName().c_str() : "?");
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        const bool enterPressed = ImGui::InputText("Prefab name##pfn", buffers.savePrefab, buffers.savePrefabSize,
            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();
        if ((ImGui::Button("Save", { 100, 0 }) || enterPressed) && strlen(buffers.savePrefab) > 0)
        {
            if (selected)
            {
                linkAndSavePrefab(selected, buffers.savePrefab);
                app->getFileSystemModule()->rebuild();
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