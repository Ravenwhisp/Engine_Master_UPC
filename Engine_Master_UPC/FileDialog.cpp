#include "Globals.h"
#include "FileDialog.h"

#include "Application.h"
#include "ModuleFileSystem.h"
#include "ModuleAssets.h"
#include "Keyboard.h"



void FileDialog::createNewFolder()
{
    std::filesystem::path newFolderPath = m_currentDirectory / "New Folder";

    int suffix = 1;
    while (std::filesystem::exists(newFolderPath))
    {
        newFolderPath = m_currentDirectory / ("New Folder (" + std::to_string(suffix++) + ")");
    }


    std::filesystem::create_directory(newFolderPath);
    app->getModuleAssets()->refresh();
}

void FileDialog::pasteFile(const std::shared_ptr<FileEntry>& directory)
{
    if (std::filesystem::exists(m_fileToManage) && std::filesystem::exists(directory->getPath()))
    {
        if (m_lastActionRequested == Command::MOVE)
            moveFile(directory.get());
    }

    app->getModuleAssets()->refresh();
    m_lastActionRequested = Command::NONE;
}

void FileDialog::importAsset(const std::shared_ptr<FileEntry>& asset)
{
    // asset->path is the .metadata path; the source file is the stem.
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
    if (!std::filesystem::exists(asset->getPath()))
        return;

    if (deleteAsset(asset.get()))
    {
        if (m_lastActionRequested != Command::NONE && asset->path == m_fileToManage)
            m_lastActionRequested = Command::NONE;
    }

    app->getModuleAssets()->refresh();
    m_selectedItem = nullptr;
}

void FileDialog::deleteFolder(const std::shared_ptr<FileEntry>& asset)
{
    if (!std::filesystem::exists(asset->getPath()))
        return;

    std::filesystem::remove_all(asset->getPath());

    if (m_lastActionRequested != Command::NONE &&
        !app->getModuleFileSystem()->exists(m_fileToManage))
    {
        m_lastActionRequested = Command::NONE;
    }

    // If we deleted the directory we were browsing, go up.
    if (m_currentDirectory == asset->path ||
        m_currentDirectory.string().find(asset->path.string()) == 0)
    {
        navigateTo(asset->path.parent_path());
    }

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
    {
        navigateTo(asset->path);
    }

}



inline bool FileDialog::moveFile(FileEntry* targetDirectory)
{
    ModuleFileSystem* fs = app->getModuleFileSystem();

    const std::filesystem::path target = targetDirectory->path / m_fileToManage.filename();

    if (fs->isDirectory(m_fileToManage))
    {
        return fs->move(m_fileToManage, target);
    }


    // m_fileToManage is a .metadata path — move both the sidecar and the source file.
    const std::filesystem::path sourcePath = m_fileToManage.parent_path() / m_fileToManage.stem();
    const std::filesystem::path sourcePathTarget = targetDirectory->path / m_fileToManage.stem();

    const bool movedMeta = fs->move(m_fileToManage, target);
    const bool movedSource = fs->move(sourcePath, sourcePathTarget);

    return movedMeta && movedSource;
}

inline bool FileDialog::deleteAsset(FileEntry* file)
{
    ModuleFileSystem* fs = app->getModuleFileSystem();

    // file->path is the .metadata path — delete both the sidecar and the source file.
    const std::filesystem::path sourcePath = file->path.parent_path() / file->path.stem();

    const bool deletedMeta = fs->remove(file->path);
    const bool deletedSource = fs->remove(sourcePath);

    return deletedMeta && deletedSource;
}


void FileDialog::drawDirectoryTree(const std::shared_ptr<FileEntry> entry)
{
    if (!entry) return;

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

void FileDialog::drawAssetGrid(const std::shared_ptr<FileEntry> directory)
{
    const float panelWidth = ImGui::GetContentRegionAvail().x;
    const float cellSize = 96.0f;
    const int   columnCount = std::max(1, static_cast<int>(panelWidth / cellSize));

    if (ImGui::BeginPopupContextWindow("##AssetGridContext",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        ImGui::Text("Create");
        ImGui::Separator();

        if (ImGui::MenuItem("New Folder"))
        {
            createNewFolder();
        }

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Text("General");
        ImGui::Separator();

        if (m_lastActionRequested != Command::NONE && ImGui::MenuItem("Paste"))
        {
            pasteFile(directory);
        }

        ImGui::EndPopup();
    }

    // Keyboard shortcuts
    const Keyboard::State& keyState = Keyboard::Get().GetState();
    if (keyState.LeftControl || keyState.RightControl)
    {
        if (keyState.X)
        {
            if (m_selectedItem) cutItem(m_selectedItem);
        }
        else if (keyState.V && m_lastActionRequested != Command::NONE)
        {
            if (m_selectedItem && std::filesystem::is_directory(m_selectedItem->path))
            {
                pasteFile(m_selectedItem);
            }
            else
            {
                pasteFile(directory);
            }
        }
    }

    ImGui::Columns(columnCount, nullptr, false);

    for (auto& asset : directory->children)
    {
        if (!asset) continue;

        ImGui::PushID(asset->displayName.c_str());

        // PROVISIONAL: replace with thumbnail image
        ImGui::Button(asset->isDirectory ? "[DIR]" : "[FILE]", ImVec2(40, 40));

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
                ImGui::SetDragDropPayload("ASSET", &asset->uid, sizeof(UID));
                ImGui::Text("Dragging %s", asset->displayName.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginPopupContextItem("ItemContext"))
            {
                ImGui::Text("Options");
                ImGui::Separator();

                const std::filesystem::path sourcePath = asset->path.parent_path() / asset->path.stem();
                const bool importable = app->getModuleAssets()->canImport(sourcePath);

                if (ImGui::MenuItem("Import", nullptr, false, importable))
                {
                    importAsset(asset);
                }

                if (ImGui::MenuItem("Cut", "Ctrl+X"))
                {
                    cutItem(asset);
                }

                if (ImGui::MenuItem("Delete", "Del"))
                    deleteItem(asset);

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
                {
                    pasteFile(asset);
                }

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