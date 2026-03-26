#include "Globals.h"
#include "WindowFileDialog.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"
#include "ModuleFileSystem.h"

#include "GameObject.h"
#include "PrefabManager.h"
#include "PrefabAsset.h"
#include "Keyboard.h"
#include "Extensions.h"
#include <CommandSaveGameObjectAsPrefab.h>
#include <CommandCreateFolder.h>
#include <CommandPasteFile.h>
#include <CommandCutItem.h>
#include <CommandImportAsset.h>
#include <CommandDeleteAsset.h>
#include <CommandDeleteFolder.h>

void WindowFileDialog::navigateTo(const std::filesystem::path& path)
{
    m_currentDirectory = path;
    m_selectedItem = nullptr;
}

void WindowFileDialog::handleAssetDoubleClick(const std::shared_ptr<FileEntry>& asset)
{
    if (asset->isDirectory)
    {
        navigateTo(asset->path);
    }
}

void WindowFileDialog::handleGameObjectDrop(const std::filesystem::path& targetDirectory)
{
    GameObject* go = app->getModuleEditor()->getSelectedGameObject();

    if (!go)
    {
        return;
    }

    CommandSaveGameObjectAsPrefab(go, targetDirectory).run();
}

void WindowFileDialog::drawDirectoryTree(const std::shared_ptr<FileEntry>& entry)
{
    if (!entry)
    {
        return;
    }

    if (ImGui::TreeNodeEx(entry->displayName.c_str()))
    {
        if (ImGui::IsItemClicked())
        {
            navigateTo(entry->path);
        }

        for (auto& child : entry->children)
        {
            if (child && child->isDirectory)
            {
                drawDirectoryTree(child);
            }
        }

        ImGui::TreePop();
    }
}

void WindowFileDialog::drawAssetGrid(const std::shared_ptr<FileEntry>& directory)
{
    const float panelWidth = ImGui::GetContentRegionAvail().x;
    const float cellSize = 96.0f;
    const int columnCount = std::max(1, static_cast<int>(panelWidth / cellSize));

    if (ImGui::BeginPopupContextWindow("##AssetGridContext",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        ImGui::Text("Create");
        ImGui::Separator();

        if (ImGui::MenuItem("New Folder"))
        {
            CommandCreateFolder(m_currentDirectory).run();
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("General");
        ImGui::Separator();

        if (m_clipboard.hasPending() && ImGui::MenuItem("Paste"))
        {
            CommandPasteFile(m_clipboard, directory->path).run();
        }

        ImGui::EndPopup();
    }

    {
        const ImVec2 dropZoneSize = ImGui::GetContentRegionAvail();
        const ImVec2 savedCursor = ImGui::GetCursorPos();

        ImGui::InvisibleButton("##goDropZone", dropZoneSize, ImGuiButtonFlags_AllowOverlap);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
            {
                GameObject* droppedGO = *static_cast<GameObject**>(payload->Data);

                if (droppedGO)
                {
                    app->getModuleEditor()->setSelectedGameObject(droppedGO);
                    handleGameObjectDrop(m_currentDirectory);
                }
            }

            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemHovered() && ImGui::GetDragDropPayload() &&
            ImGui::GetDragDropPayload()->IsDataType("GAME_OBJECT"))
        {
            const ImVec2 pMin = ImGui::GetItemRectMin();
            const ImVec2 pMax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(pMin, pMax, IM_COL32(50, 160, 50, 40));
            ImGui::GetWindowDrawList()->AddRect(pMin, pMax, IM_COL32(50, 200, 50, 120), 0.f, 0, 2.f);
        }

        ImGui::SetCursorPos(savedCursor);
    }

    const Keyboard::State& keyState = Keyboard::Get().GetState();

    if (keyState.LeftControl || keyState.RightControl)
    {
        if (keyState.X && m_selectedItem)
        {
            CommandCutItem(m_clipboard, m_selectedItem->path).run();
        }
        else if (keyState.V && m_clipboard.hasPending())
        {
            const std::filesystem::path& pasteTarget = (m_selectedItem && std::filesystem::is_directory(m_selectedItem->path)) ? m_selectedItem->path : directory->path;
            CommandPasteFile(m_clipboard, pasteTarget).run();
        }
    }

    ImGui::Columns(columnCount, nullptr, false);

    for (auto& asset : directory->children)
    {
        if (!asset)
        {
            continue;
        }

        ImGui::PushID(asset->displayName.c_str());

        auto realPath = asset->path;
        realPath.replace_extension();

        const bool isPrefab = !asset->isDirectory && (realPath.extension() == PREFAB_EXTENSION || realPath.extension() == GLTF_EXTENSION);

        if (isPrefab)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.30f, 0.10f, 1.f));
        }

        ImGui::Button(asset->isDirectory ? "[DIR]" : (isPrefab ? "[P]" : "[FILE]"), ImVec2(40, 40));

        if (isPrefab)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemClicked())
        {
            m_selectedItem = asset;
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            handleAssetDoubleClick(asset);
        }

        if (!asset->isDirectory)
        {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                const std::filesystem::path sourcePath = asset->path.parent_path() / asset->path.stem();

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
                PrefabUI::drawFileDialogItemContextMenu(asset->path, m_showVariantModal, m_renamingPrefab, buffers);
            }

            if (ImGui::BeginPopupContextItem("ItemContext"))
            {
                ImGui::Text("Options");
                ImGui::Separator();

                const std::filesystem::path originalPath = asset->path.parent_path() / asset->path.stem();
                const bool canImport = app->getModuleAssets()->canImport(originalPath);

                if (ImGui::MenuItem("Import", nullptr, false, canImport))
                {
                    CommandImportAsset(originalPath, asset->uid).run();
                }

                if (ImGui::MenuItem("Cut", "Ctrl+X"))
                {
                    CommandCutItem(m_clipboard, asset->path).run();
                }

                if (ImGui::MenuItem("Delete", "Del"))
                {
                    if (m_clipboard.fileToManage == asset->path)
                    {
                        m_clipboard.clear();
                    }

                    {
                        CommandDeleteAsset deleteAction(asset->path);
                        deleteAction.run();

                        if (deleteAction.getResult())
                        {
                            m_selectedItem = nullptr;
                        }
                    }

                    app->getModuleAssets()->refresh();
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
                {
                    CommandCutItem(m_clipboard, asset->path).run();
                }

                if (ImGui::MenuItem("Delete Folder", "Del"))
                {
                    if (!app->getModuleFileSystem()->exists(m_clipboard.fileToManage))
                    {
                        m_clipboard.clear();
                    }

                    const std::filesystem::path redirect = CommandDeleteFolder(asset->path, m_currentDirectory).getResult();

                    if (!redirect.empty())
                    {
                        navigateTo(redirect);
                    }

                    m_selectedItem = nullptr;
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Text("General");
                ImGui::Separator();

                if (m_clipboard.hasPending() && m_clipboard.fileToManage != asset->path && ImGui::MenuItem("Paste", "Ctrl+V"))
                {
                    CommandPasteFile(m_clipboard, asset->path).run();
                }

                ImGui::EndPopup();
            }
        }

        ImGui::TextWrapped("%s", asset->displayName.c_str());
        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    PrefabUI::FileDialogBuffers buffers = buildFileDialogBuffers();
    PrefabUI::drawFileDialogModals(m_showVariantModal, m_showSavePrefabModal, m_renamingPrefab, buffers);
}

void WindowFileDialog::drawInternal()
{
    ImGui::BeginChild("LeftPanel", ImVec2(250, 0), true);
    drawDirectoryTree(app->getModuleAssets()->getRoot());
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);

    if (std::shared_ptr<FileEntry> dir = app->getModuleAssets()->getEntry(m_currentDirectory))
    {
        drawAssetGrid(dir);
    }

    ImGui::EndChild();
}

PrefabUI::FileDialogBuffers WindowFileDialog::buildFileDialogBuffers()
{
    PrefabUI::FileDialogBuffers buffers;
    buffers.variantSource = m_variantSrcBuf;
    buffers.variantSourceSize = sizeof(m_variantSrcBuf);
    buffers.variantDest = m_variantDstBuf;
    buffers.variantDestSize = sizeof(m_variantDstBuf);
    buffers.renameSource = m_renameSrcBuf;
    buffers.renameSourceSize = sizeof(m_renameSrcBuf);
    buffers.renameDest = m_renameDstBuf;
    buffers.renameDestSize = sizeof(m_renameDstBuf);
    buffers.savePrefab = m_savePrefabNameBuf;
    buffers.savePrefabSize = sizeof(m_savePrefabNameBuf);
    return buffers;
}
