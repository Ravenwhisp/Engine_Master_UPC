#include "Globals.h"
#include "FileDialog.h"

#include "Application.h"
#include "FileSystemModule.h"
#include "AssetsModule.h"


// ---------------------------------------------------------
// Actions
// ---------------------------------------------------------

void FileDialog::createNewFolder()
{
    std::filesystem::path newFolderPath = m_currentDirectory / "New Folder";

    int suffix = 1;
    while (std::filesystem::exists(newFolderPath))
    {
        newFolderPath = m_currentDirectory / ("New Folder (" + std::to_string(suffix++) + ")");
    }

    std::filesystem::create_directory(newFolderPath);
    app->getFileSystemModule()->rebuild();
}

void FileDialog::pasteFile(const std::shared_ptr<FileEntry>& directory)
{
    if (m_lastActionRequested == Command::MOVE)
    {
        moveFile(directory.get());
    }

    app->getFileSystemModule()->rebuild();
    m_lastActionRequested = Command::NONE;
}

void FileDialog::importAsset(const std::shared_ptr<FileEntry>& asset)
{
    std::filesystem::path originalPath = asset->path.parent_path() / asset->path.stem();
    app->getAssetModule()->import(originalPath);
}

void FileDialog::cutItem(const std::shared_ptr<FileEntry>& asset)
{
    m_lastActionRequested = Command::MOVE;
    m_fileToManage = asset->path;
}

void FileDialog::deleteFolder(const std::shared_ptr<FileEntry>& asset)
{
    if (!std::filesystem::exists(asset->getPath()))
    {
        return;
    }

    std::filesystem::remove_all(asset->getPath());
    app->getFileSystemModule()->rebuild();

    // If we deleted the directory we were browsing, go up
    if (m_currentDirectory == asset->path ||
        m_currentDirectory.string().find(asset->path.string()) == 0)
    {
        navigateTo(asset->path.parent_path());
    }

    m_selectedItem = nullptr;
}



void FileDialog::navigateTo(const std::filesystem::path& path)
{
    m_currentDirectory = path;
}

void FileDialog::handleAssetDoubleClick(const std::shared_ptr<FileEntry>& asset)
{
    if (asset->isDirectory)
    {
        navigateTo(asset->path);
    }
}


void FileDialog::drawDirectoryTree(const std::shared_ptr<FileEntry> entry)
{
    std::string nodeName = entry->displayName;

    if (ImGui::TreeNodeEx(nodeName.c_str()))
    {
        if (ImGui::IsItemClicked())
        {
            navigateTo(entry->path);
        }

        for (auto& child : entry->children)
        {
            if (child != nullptr && child->isDirectory)
            {
                drawDirectoryTree(child);
            }
        }
        ImGui::TreePop();
    }
}

void FileDialog::drawAssetGrid(const std::shared_ptr<FileEntry> directory)
{
    float padding = 16.0f;  // You could have these
    float cellSize = 96.0f; // values saved somewhere more visible...

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1)
    {
        columnCount = 1;
    }

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

    ImGui::Columns(columnCount, nullptr, false);

    for (auto& asset : directory->children)
    {
        if (asset == nullptr)
        {
            continue;
        }

        ImGui::PushID(asset->displayName.c_str());

        // PROVISIONAL: This will be changed for an image
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

                std::filesystem::path originalPath = asset->path.parent_path() / asset->path.stem();
                Importer* importer = app->getFileSystemModule()->findImporter(originalPath);

                if (ImGui::MenuItem("Import", nullptr, false, importer != nullptr))
                {
                    importAsset(asset);
                }

                if (ImGui::MenuItem("Cut", "Ctrl + X", false, true))
                {
                    cutItem(asset);
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

                if (ImGui::MenuItem("Cut Folder", "Ctrl + X", false, true))
                {
                    cutItem(asset);
                }

                if (ImGui::MenuItem("Delete Folder"))
                {
                    deleteFolder(asset);
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

inline bool FileDialog::moveFile(FileEntry* targetDirectory)
{
    std::string fileString = m_fileToManage.string();
    const char* file = fileString.c_str();

    std::string targetNameString = (targetDirectory->path / m_fileToManage.filename()).string();
    const char* targetName = targetNameString.c_str();

    if (app->getFileSystemModule()->isDirectory(file))
    {
        return app->getFileSystemModule()->move(file, targetName);
    }
    else
    {
        // The file that we have is the metadata; we have to move its asset as well, which should be on the same folder

        bool moveMetadata = app->getFileSystemModule()->move(file, targetName);

        std::string assetPathString = (m_fileToManage.parent_path() / m_fileToManage.stem()).string(); // stem() is the file name, takes out the .metadata at the end
        const char* assetPath = assetPathString.c_str();

        std::string assetTargetNameString = (targetDirectory->path / m_fileToManage.stem()).string();
        const char* assetTargetName = assetTargetNameString.c_str();

        bool moveFile = app->getFileSystemModule()->move(assetPath, assetTargetName);

        return moveFile && moveMetadata;
    }
}


void FileDialog::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr(),
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    ImGui::BeginChild("LeftPanel", ImVec2(250, 0), true);
    drawDirectoryTree(app->getFileSystemModule()->getRoot());
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
    if (std::shared_ptr<FileEntry> dir = app->getFileSystemModule()->getEntry(m_currentDirectory))
    {
        drawAssetGrid(dir);
    }
    ImGui::EndChild();
    ImGui::End();
}