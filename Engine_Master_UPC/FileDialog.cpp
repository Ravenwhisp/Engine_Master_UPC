#include "Globals.h"
#include "FileDialog.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "PrefabManager.h"
#include "PrefabAsset.h"
#include "Keyboard.h"
#include "Extensions.h"
#include <FileIO.h>


// ---------------------------------------------------------------------------
// handleGameObjectDrop
// Called when a "GAME_OBJECT" drag-drop payload is released over the asset
// grid.  Creates a .prefab file in targetDirectory named after the GO and
// links the GO to it — exactly as dragging a GameObject into the Project
// window works in Unity.
// ---------------------------------------------------------------------------
void FileDialog::handleGameObjectDrop(const std::filesystem::path& targetDirectory)
{
    GameObject* go = app->getModuleEditor()->getSelectedGameObject();
    if (!go) return;

    // Build a unique save path: targetDir / GoName.prefab
    std::filesystem::path basePath = targetDirectory / (go->GetName() + ".prefab");
    std::filesystem::path savePath = basePath;
    int suffix = 1;
    while (FileIO::exists(savePath))
    {
        savePath = targetDirectory /
            (go->GetName() + "_" + std::to_string(suffix++) + ".prefab");
    }

    // createPrefab writes the file AND links go->GetPrefabInfo() in-place.
    if (!PrefabManager::createPrefab(go, savePath))
    {
        DEBUG_ERROR("[FileDialog] Failed to create prefab at '%s'.",
            savePath.string().c_str());
        return;
    }

    app->getModuleAssets()->refresh();
}


void FileDialog::createNewFolder()
{
    std::filesystem::path newFolderPath = m_currentDirectory / "New Folder";

    int suffix = 1;
    while (std::filesystem::exists(newFolderPath))
        newFolderPath = m_currentDirectory / ("New Folder (" + std::to_string(suffix++) + ")");

    std::filesystem::create_directory(newFolderPath);
    app->getModuleAssets()->refresh();
}

void FileDialog::pasteFile(const std::shared_ptr<FileEntry>& directory)
{
    if (std::filesystem::exists(m_fileToManage) && std::filesystem::exists(directory->getPath()))
        if (m_lastActionRequested == Command::MOVE)
            moveFile(directory.get());

    app->getModuleAssets()->refresh();
    m_lastActionRequested = Command::NONE;
}

void FileDialog::importAsset(const std::shared_ptr<FileEntry>& asset)
{
    const std::filesystem::path sourcePath = asset->path.parent_path() / asset->path.stem();
    app->getModuleAssets()->importAsset(sourcePath, asset->uid);
}

void FileDialog::cutItem(const std::shared_ptr<FileEntry>& asset)
{
    m_lastActionRequested = Command::MOVE;
    m_fileToManage = asset->path;
}

void FileDialog::deleteItem(const std::shared_ptr<FileEntry>& asset)
{
    if (!std::filesystem::exists(asset->getPath())) return;

    if (deleteAsset(asset.get()))
        if (m_lastActionRequested != Command::NONE && asset->path == m_fileToManage)
            m_lastActionRequested = Command::NONE;

    app->getModuleAssets()->refresh();
    m_selectedItem = nullptr;
}

void FileDialog::deleteFolder(const std::shared_ptr<FileEntry>& asset)
{
    if (!std::filesystem::exists(asset->getPath())) return;

    std::filesystem::remove_all(asset->getPath());

    if (m_lastActionRequested != Command::NONE &&
        !FileIO::exists(m_fileToManage))
        m_lastActionRequested = Command::NONE;

    if (m_currentDirectory == asset->path ||
        m_currentDirectory.string().find(asset->path.string()) == 0)
        navigateTo(asset->path.parent_path());

    app->getModuleAssets()->refresh();
    m_selectedItem = nullptr;
}

void FileDialog::navigateTo(const std::filesystem::path& path)
{
    m_currentDirectory = path;
    m_selectedItem = nullptr;
}

void FileDialog::handleAssetDoubleClick(const std::shared_ptr<FileEntry>& asset)
{
    if (asset->isDirectory)
        navigateTo(asset->path);
}

inline bool FileDialog::moveFile(FileEntry* targetDirectory)
{
    const std::filesystem::path target = targetDirectory->path / m_fileToManage.filename();

    if (FileIO::isDirectory(m_fileToManage))
        return FileIO::move(m_fileToManage, target);

    const std::filesystem::path sourcePath = m_fileToManage.parent_path() / m_fileToManage.stem();
    const std::filesystem::path sourcePathTarget = targetDirectory->path / m_fileToManage.stem();

    const bool movedMeta = FileIO::move(m_fileToManage, target);
    const bool movedSource = FileIO::move(sourcePath, sourcePathTarget);
    return movedMeta && movedSource;
}

inline bool FileDialog::deleteAsset(FileEntry* file)
{
    const std::filesystem::path sourcePath = file->path.parent_path() / file->path.stem();
    const bool deletedMeta = FileIO::remove(file->path);
    const bool deletedSource = FileIO::remove(sourcePath);
    return deletedMeta && deletedSource;
}

void FileDialog::drawDirectoryTree(const std::shared_ptr<FileEntry> entry)
{
    if (!entry) return;

    if (ImGui::TreeNodeEx(entry->displayName.c_str()))
    {
        if (ImGui::IsItemClicked())
            navigateTo(entry->path);

        for (auto& child : entry->children)
            if (child && child->isDirectory)
                drawDirectoryTree(child);

        ImGui::TreePop();
    }
}

void FileDialog::drawAssetGrid(const std::shared_ptr<FileEntry> directory)
{
    const float panelWidth = ImGui::GetContentRegionAvail().x;
    const float cellSize = 96.0f;
    const int   columnCount = std::max(1, static_cast<int>(panelWidth / cellSize));

    // ── Background context menu ──────────────────────────────────────────────
    if (ImGui::BeginPopupContextWindow("##AssetGridContext",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        ImGui::Text("Create");
        ImGui::Separator();
        if (ImGui::MenuItem("New Folder"))
            createNewFolder();

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Text("General");
        ImGui::Separator();
        if (m_lastActionRequested != Command::NONE && ImGui::MenuItem("Paste"))
            pasteFile(directory);

        ImGui::EndPopup();
    }

    // ── Full-panel drop target for "GAME_OBJECT" payloads ────────────────────
    // BeginDragDropTarget only fires on hovered *items*, not on child-window
    // backgrounds.  The fix is an InvisibleButton that covers the entire
    // available panel area — it is always the last hovered item when the mouse
    // is anywhere over the grid.  We save and restore the cursor so the column
    // layout that follows renders in exactly the same position.
    {
        const ImVec2 dropZoneSize = ImGui::GetContentRegionAvail();
        const ImVec2 savedCursor = ImGui::GetCursorPos();

        // Reserve the full area as an interactable item.
        ImGui::InvisibleButton("##goDropZone", dropZoneSize,
            ImGuiButtonFlags_AllowOverlap);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("GAME_OBJECT"))
            {
                // The Hierarchy sets the payload as &gameObject (a local
                // GameObject* variable), so the data is a GameObject**.
                GameObject* droppedGO =
                    *static_cast<GameObject**>(payload->Data);
                if (droppedGO)
                {
                    app->getModuleEditor()->setSelectedGameObject(droppedGO);
                    handleGameObjectDrop(m_currentDirectory);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Draw a subtle highlight while a draggable item hovers over the panel.
        if (ImGui::IsItemHovered() && ImGui::GetDragDropPayload() &&
            ImGui::GetDragDropPayload()->IsDataType("GAME_OBJECT"))
        {
            const ImVec2 pMin = ImGui::GetItemRectMin();
            const ImVec2 pMax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(
                pMin, pMax, IM_COL32(50, 160, 50, 40));
            ImGui::GetWindowDrawList()->AddRect(
                pMin, pMax, IM_COL32(50, 200, 50, 120), 0.f, 0, 2.f);
        }

        // Restore the cursor so the column grid renders on top of the drop zone.
        ImGui::SetCursorPos(savedCursor);
    }

    // ── Keyboard shortcuts ───────────────────────────────────────────────────
    const Keyboard::State& keyState = Keyboard::Get().GetState();
    if (keyState.LeftControl || keyState.RightControl)
    {
        if (keyState.X && m_selectedItem)
            cutItem(m_selectedItem);
        else if (keyState.V && m_lastActionRequested != Command::NONE)
        {
            if (m_selectedItem && std::filesystem::is_directory(m_selectedItem->path))
                pasteFile(m_selectedItem);
            else
                pasteFile(directory);
        }
    }

    // ── Asset grid items ─────────────────────────────────────────────────────
    ImGui::Columns(columnCount, nullptr, false);

    for (auto& asset : directory->children)
    {
        if (!asset) continue;

        ImGui::PushID(asset->displayName.c_str());
        auto realPath = asset->path;
        realPath.replace_extension();

        const bool isPrefab = (!asset->isDirectory && (realPath.extension() == PREFAB_EXTENSION || realPath.extension() == GLTF_EXTENSION));

        if (isPrefab)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.30f, 0.10f, 1.f));

        ImGui::Button(asset->isDirectory ? "[DIR]" : (isPrefab ? "[P]" : "[FILE]"), ImVec2(40, 40));

        if (isPrefab)
            ImGui::PopStyleColor();

        if (ImGui::IsItemClicked())
            m_selectedItem = asset;

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            handleAssetDoubleClick(asset);

        if (!asset->isDirectory)
        {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                // Resolve the source file path from the .metadata sidecar path.
                const std::filesystem::path sourcePath = asset->path.parent_path() / asset->path.stem();

                // Emit a specialised payload for prefab files so the Hierarchy
                // can accept and instantiate them directly.  All other assets
                // keep the generic "ASSET" (UID) payload.
                if (sourcePath.extension() == PREFAB_EXTENSION || sourcePath.extension() == GLTF_EXTENSION)
                {
                    const std::string pathStr = sourcePath.string();
                    ImGui::SetDragDropPayload("PREFAB_ASSET", pathStr.c_str(), pathStr.size() + 1);
                    ImGui::Text("[Prefab]  %s", asset->displayName.c_str());
                }
                else
                {
                    ImGui::SetDragDropPayload("ASSET", &asset->uid, sizeof(MD5Hash));
                    ImGui::Text("Dragging %s", asset->displayName.c_str());
                }
                ImGui::EndDragDropSource();
            }

            if (isPrefab)
            {
                PrefabUI::FileDialogBuffers buffers = buildFileDialogBuffers();
                PrefabUI::drawFileDialogItemContextMenu(asset->path.stem().string(), m_showVariantModal, m_renamingPrefab, buffers);
            }
                if (ImGui::BeginPopupContextItem("ItemContext"))
                {
                    ImGui::Text("Options");
                    ImGui::Separator();

                    std::filesystem::path originalPath = asset->path.parent_path() / asset->path.stem();
                    bool canImporter = app->getModuleAssets()->canImport(originalPath);

                    if (ImGui::MenuItem("Import", nullptr, false, canImporter))
                    {
                        importAsset(asset);
                    }

                    if (ImGui::MenuItem("Cut", "Ctrl + X", false, true))
                    {
                        cutItem(asset);
                    }

                    if (ImGui::MenuItem("Delete", "Del", false, true))
                    {
                        deleteItem(asset);
                    }

                    ImGui::EndPopup();
                }
            
        }
        else
        {
            if (ImGui::BeginPopupContextItem("DirContext"))
            {
                ImGui::Text("Folder: %s", asset->displayName.c_str());
                ImGui::Separator();
                m_selectedItem = asset;

                if (ImGui::MenuItem("Cut Folder", "Ctrl+X"))
                    cutItem(asset);
                if (ImGui::MenuItem("Delete Folder", "Del"))
                    deleteFolder(asset);

                ImGui::Spacing(); ImGui::Spacing();
                ImGui::Text("General");
                ImGui::Separator();

                if (m_lastActionRequested != Command::NONE &&
                    m_fileToManage != asset->path &&
                    ImGui::MenuItem("Paste", "Ctrl+V"))
                    pasteFile(asset);

                ImGui::EndPopup();
            }
        }

        ImGui::TextWrapped("%s", asset->displayName.c_str());
        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);
}

void FileDialog::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr(), ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    ImGui::BeginChild("LeftPanel", ImVec2(250, 0), true);
    drawDirectoryTree(app->getModuleAssets()->getRoot());
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
    if (std::shared_ptr<FileEntry> dir = app->getModuleAssets()->getEntry(m_currentDirectory))
        drawAssetGrid(dir);
    ImGui::EndChild();

    ImGui::End();
}

PrefabUI::FileDialogBuffers FileDialog::buildFileDialogBuffers()
{
    PrefabUI::FileDialogBuffers buffers;
    buffers.variantSource = m_variantSrcBuf;      buffers.variantSourceSize = sizeof(m_variantSrcBuf);
    buffers.variantDest = m_variantDstBuf;      buffers.variantDestSize = sizeof(m_variantDstBuf);
    buffers.renameSource = m_renameSrcBuf;       buffers.renameSourceSize = sizeof(m_renameSrcBuf);
    buffers.renameDest = m_renameDstBuf;       buffers.renameDestSize = sizeof(m_renameDstBuf);
    buffers.savePrefab = m_savePrefabNameBuf;  buffers.savePrefabSize = sizeof(m_savePrefabNameBuf);
    return buffers;
}