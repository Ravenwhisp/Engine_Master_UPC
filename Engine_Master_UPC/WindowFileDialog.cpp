#include "Globals.h"
#include "WindowFileDialog.h"

#include "Application.h"
#include "Asset.h"
#include "AssetReference.h"
#include "ContentRegistry.h"
#include "FileIO.h"
#include "GameObject.h"
#include "Keyboard.h"
#include "Metadata.h"
#include "ModuleAssets.h"
#include "ModuleEditor.h"

#include "CommandCreateFolder.h"
#include "CommandCutItem.h"
#include "CommandDeleteAsset.h"
#include "CommandDeleteFolder.h"
#include "CommandImportAsset.h"
#include "CommandPasteFile.h"
#include "CommandSaveGameObjectAsPrefab.h"
#include "CommandCreateDataContainer.h"

#include "GenericTypeFactory.h"

#include <algorithm>
#include <string>

void WindowFileDialog::navigateTo(const std::filesystem::path& path)
{
    m_currentDirectory = path.lexically_normal();
    m_selectedPath.clear();
    m_selectedAsset = INVALID_UID;
}

void WindowFileDialog::handleAssetClick(const AssetEntry& asset)
{
    if (!isValidUID(asset.uid))
    {
        return;
    }

    AssetReference ref(asset.uid);
    std::shared_ptr<Asset> assetResource = app->getModuleAssets()->load<Asset>(ref);

    app->getModuleEditor()->setSelectedAsset(assetResource);

    m_selectedAsset = asset.uid;
}

void WindowFileDialog::handleDirectoryClick(DirectoryEntry* directory)
{
    if (!directory)
    {
        return;
    }

    m_selectedPath = directory->path;
    m_selectedAsset = INVALID_UID;
}

void WindowFileDialog::handleDirectoryDoubleClick(DirectoryEntry* directory)
{
    if (!directory)
    {
        return;
    }

    navigateTo(directory->path);
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

std::filesystem::path WindowFileDialog::getAssetSourcePath(
    const DirectoryEntry& directory,
    const AssetEntry& asset
) const
{
    return (directory.path / asset.displayName).lexically_normal();
}

std::filesystem::path WindowFileDialog::getAssetMetaPath(
    const DirectoryEntry& directory,
    const AssetEntry& asset
) const
{
    std::filesystem::path metaPath = getAssetSourcePath(directory, asset);
    Metadata::getMetadataPath(metaPath);
    return metaPath.lexically_normal();
}

void WindowFileDialog::drawDirectoryTree(DirectoryEntry* directory)
{
    if (!directory)
    {
        return;
    }

    if (ImGui::TreeNodeEx(directory->displayName.c_str()))
    {
        if (ImGui::IsItemClicked())
        {
            navigateTo(directory->path);
        }

        for (const auto& child : directory->directories)
        {
            drawDirectoryTree(child.get());
        }

        ImGui::TreePop();
    }
}

void WindowFileDialog::drawDirectoryItem(DirectoryEntry* directory)
{
    if (!directory)
    {
        return;
    }

    ImGui::PushID(directory->path.string().c_str());

    ImGui::Button("[DIR]", ImVec2(40, 40));

    if (ImGui::IsItemClicked())
    {
        handleDirectoryClick(directory);
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        handleDirectoryDoubleClick(directory);
    }

    if (ImGui::BeginPopupContextItem("DirContext"))
    {
        ImGui::Text("Folder: %s", directory->displayName.c_str());
        ImGui::Separator();

        m_selectedPath = directory->path;
        m_selectedAsset = INVALID_UID;

        if (ImGui::MenuItem("Cut Folder", "Ctrl+X"))
        {
            CommandCutItem(m_clipboard, directory->path).run();
        }

        if (ImGui::MenuItem("Delete Folder", "Del"))
        {
            if (!FileIO::exists(m_clipboard.fileToManage))
            {
                m_clipboard.clear();
            }

            const std::filesystem::path redirect =
                CommandDeleteFolder(directory->path, m_currentDirectory).getResult();

            if (!redirect.empty())
            {
                navigateTo(redirect);
            }

            m_selectedPath.clear();
            m_selectedAsset = INVALID_UID;
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("General");
        ImGui::Separator();

        if (m_clipboard.hasPending() &&
            m_clipboard.fileToManage != directory->path &&
            ImGui::MenuItem("Paste", "Ctrl+V"))
        {
            CommandPasteFile(m_clipboard, directory->path).run();
        }

        ImGui::EndPopup();
    }

    ImGui::TextWrapped("%s", directory->displayName.c_str());
    ImGui::NextColumn();

    ImGui::PopID();
}

void WindowFileDialog::drawAssetItem(DirectoryEntry* directory, const AssetEntry& asset)
{
    if (!directory)
    {
        return;
    }

    const std::filesystem::path sourcePath = getAssetSourcePath(*directory, asset);
    const std::filesystem::path metaPath = getAssetMetaPath(*directory, asset);

    const bool isPrefab =
        sourcePath.extension() == PREFAB_EXTENSION ||
        sourcePath.extension() == GLTF_EXTENSION;

    ImGui::PushID(static_cast<int>(asset.uid));

    const bool hasSubAssets = !asset.subAssets.empty();

    if (hasSubAssets)
    {
        const bool isExpanded = m_expandedAssets.count(asset.uid) > 0;
        if (ImGui::ArrowButton("##expand", isExpanded ? ImGuiDir_Down : ImGuiDir_Right))
        {
            if (isExpanded)
                m_expandedAssets.erase(asset.uid);
            else
                m_expandedAssets.insert(asset.uid);
        }
        ImGui::SameLine();
    }

    if (isPrefab)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.30f, 0.10f, 1.f));
    }

    ImGui::Button(isPrefab ? "[P]" : "[FILE]", ImVec2(40, 40));

    if (isPrefab)
    {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        handleAssetClick(asset);
        m_selectedPath = metaPath;
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        if (isPrefab)
        {
            const std::string pathStr = sourcePath.string();
            ImGui::SetDragDropPayload("PREFAB_ASSET", pathStr.c_str(), pathStr.size() + 1);
            ImGui::Text("[Prefab] %s", asset.displayName.c_str());
        }
        else
        {
            ImGui::SetDragDropPayload("ASSET", &asset.uid, sizeof(UID));
            ImGui::Text("Dragging %s", asset.displayName.c_str());
        }

        ImGui::EndDragDropSource();
    }

    if (isPrefab)
    {
        PrefabUI::FileDialogBuffers buffers = buildFileDialogBuffers();
        PrefabUI::drawFileDialogItemContextMenu(
            metaPath,
            m_showVariantModal,
            m_renamingPrefab,
            buffers
        );
    }

    if (ImGui::BeginPopupContextItem("ItemContext"))
    {
        ImGui::Text("Options");
        ImGui::Separator();

        const bool canImport = app->getModuleAssets()->canImport(sourcePath);

        if (ImGui::MenuItem("Import", nullptr, false, canImport))
        {
            CommandImportAsset(sourcePath, asset.uid).run();
        }

        const bool isGltf = sourcePath.extension() == GLTF_EXTENSION;
        if (isGltf && ImGui::MenuItem("Create State Machine"))
        {
            app->getModuleAssets()->createStateMachineFromGltf(sourcePath);
        }

        if (ImGui::MenuItem("Rename..."))
        {
            m_renamingAsset = true;
            const std::string srcStr = sourcePath.string();
            strncpy_s(m_renameSrcBuf, sizeof(m_renameSrcBuf), srcStr.c_str(), sizeof(m_renameSrcBuf) - 1);
            strncpy_s(m_renameDstBuf, sizeof(m_renameDstBuf), srcStr.c_str(), sizeof(m_renameDstBuf) - 1);
        }

        if (ImGui::MenuItem("Cut", "Ctrl+X"))
        {
            CommandCutItem(m_clipboard, metaPath).run();
        }

        if (ImGui::MenuItem("Delete", "Del"))
        {
            if (m_clipboard.fileToManage == metaPath)
            {
                m_clipboard.clear();
            }

            CommandDeleteAsset deleteAction(metaPath);
            deleteAction.run();

            if (deleteAction.getResult())
            {
                m_selectedPath.clear();
                m_selectedAsset = INVALID_UID;
            }

            app->getModuleAssets()->unregisterAsset(metaPath.parent_path() / metaPath.stem());
        }

        ImGui::EndPopup();
    }

    ImGui::TextWrapped("%s", asset.displayName.c_str());
    ImGui::NextColumn();

    ImGui::PopID();
}

void WindowFileDialog::drawSubAssetItem(const AssetEntry& subAsset)
{
    if (!isValidUID(subAsset.uid))
        return;

    ImGui::PushID(static_cast<int>(subAsset.uid));

    ImGui::Button("[S]", ImVec2(40, 40));

    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        AssetReference ref(subAsset.uid);
        std::shared_ptr<Asset> assetResource = app->getModuleAssets()->load<Asset>(ref);
        app->getModuleEditor()->setSelectedAsset(assetResource);
        m_selectedAsset = subAsset.uid;
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("ASSET", &subAsset.uid, sizeof(UID));
        ImGui::Text("Dragging %s", subAsset.displayName.c_str());
        ImGui::EndDragDropSource();
    }

    ImGui::TextWrapped("%s", subAsset.displayName.c_str());
    ImGui::NextColumn();

    ImGui::PopID();
}

void WindowFileDialog::drawAssetGrid(DirectoryEntry* directory)
{
    if (!directory)
    {
        return;
    }

    const float panelWidth = ImGui::GetContentRegionAvail().x;
    const float cellSize = 96.0f;
    const int columnCount = std::max(1, (static_cast<int>(panelWidth / cellSize)));

    if (ImGui::BeginPopupContextWindow(
        "##AssetGridContext",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        ImGui::Text("Create");
        ImGui::Separator();

        if (ImGui::MenuItem("New Folder"))
        {
            CommandCreateFolder(m_currentDirectory).run();
        }

        const auto& dcRegistry = DataContainerFactory::getAllRegistered();
        if (!dcRegistry.empty())
        {
            ImGui::Spacing();
            if (ImGui::BeginMenu("New Data Asset"))
            {
                for (const auto& entry : dcRegistry)
                {
                    if (ImGui::MenuItem(entry.displayName.c_str()))
                    {
                        std::string assetName = entry.displayName + "_New";
                        CommandCreateDataContainer(m_currentDirectory, entry.name, assetName).run();
                    }
                }
                ImGui::EndMenu();
            }
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

    if (ImGui::IsItemHovered() &&
        ImGui::GetDragDropPayload() &&
        ImGui::GetDragDropPayload()->IsDataType("GAME_OBJECT"))
    {
        const ImVec2 pMin = ImGui::GetItemRectMin();
        const ImVec2 pMax = ImGui::GetItemRectMax();

        ImGui::GetWindowDrawList()->AddRectFilled(pMin, pMax, IM_COL32(50, 160, 50, 40));
        ImGui::GetWindowDrawList()->AddRect(pMin, pMax, IM_COL32(50, 200, 50, 120), 0.f, 0, 2.f);
    }

    ImGui::SetCursorPos(savedCursor);

    const Keyboard::State& keyState = Keyboard::Get().GetState();

    if (keyState.LeftControl || keyState.RightControl)
    {
        if (keyState.X && !m_selectedPath.empty())
        {
            CommandCutItem(m_clipboard, m_selectedPath).run();
        }
        else if (keyState.V && m_clipboard.hasPending())
        {
            const std::filesystem::path pasteTarget =
                !m_selectedPath.empty() && std::filesystem::is_directory(m_selectedPath)
                ? m_selectedPath
                : directory->path;

            CommandPasteFile(m_clipboard, pasteTarget).run();
        }
    }

    ImGui::Columns(columnCount, nullptr, false);

    for (const auto& childDirectory : directory->directories)
    {
        drawDirectoryItem(childDirectory.get());
    }

    for (const AssetEntry& asset : directory->assets)
    {
        drawAssetItem(directory, asset);
        if (!asset.subAssets.empty() && m_expandedAssets.count(asset.uid))
        {
            for (const AssetEntry& sub : asset.subAssets)
            {
                drawSubAssetItem(sub);
            }
        }
    }

    ImGui::Columns(1);

    PrefabUI::FileDialogBuffers buffers = buildFileDialogBuffers();
    PrefabUI::drawFileDialogModals(
        m_showVariantModal,
        m_showSavePrefabModal,
        m_renamingPrefab,
        m_renamingAsset,
        buffers
    );
}

void WindowFileDialog::drawInternal()
{
    ContentRegistry* registry = app->getModuleAssets()->getContentRegistry();

    if (!registry)
    {
        return;
    }

    ImGui::BeginChild("LeftPanel", ImVec2(250, 0), true);
    drawDirectoryTree(registry->getRoot());
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);

    if (DirectoryEntry* directory = registry->getDirectory(m_currentDirectory))
    {
        drawAssetGrid(directory);
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