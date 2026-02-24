#include "Globals.h"
#include "FileDialog.h"

#include "Application.h"
#include "FileSystemModule.h"
#include "AssetsModule.h"

void FileDialog::drawDirectoryTree(const std::shared_ptr<FileEntry> entry)
{
    std::string nodeName = entry->displayName;

    if (ImGui::TreeNodeEx(nodeName.c_str()))
    {

        if (ImGui::IsItemClicked())
        {
            m_currentDirectory = entry->path;
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
    float padding = 16.0f;
    float cellSize = 96.0f;

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
            std::filesystem::path newFolderPath = m_currentDirectory / "New Folder";

            int suffix = 1;
            while (std::filesystem::exists(newFolderPath))
            {
                newFolderPath = m_currentDirectory / ("New Folder (" + std::to_string(suffix++) + ")");
            }

            std::filesystem::create_directory(newFolderPath);
            app->getFileSystemModule()->rebuild();
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
            if (asset->isDirectory)
            {
                m_currentDirectory = asset->path;
            }
        }
        if (!asset->isDirectory) {

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {

                ImGui::SetDragDropPayload("ASSET", &asset->uid, sizeof(UID));
                ImGui::Text("Dragging %s", asset->displayName);
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
                    app->getAssetModule()->import(originalPath);
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

                if (ImGui::MenuItem("Delete Folder"))
                {
                    if (std::filesystem::exists(m_selectedItem->getPath()))
                    {
                        std::filesystem::remove_all(m_selectedItem->getPath());
                        app->getFileSystemModule()->rebuild();

                        // If we deleted the directory we were browsing, go up
                        if (m_currentDirectory == m_selectedItem->path || m_currentDirectory.string().find(m_selectedItem->path.string()) == 0)
                        {
                            m_currentDirectory = m_selectedItem->path.parent_path();
                        }

                        m_selectedItem = nullptr;
                    }
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



FileDialog::FileDialog()
{
    //m_currentDirectory = app->getFileSystemModule()->getRoot()->path;
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
    ImGui::BeginChild("RightPane", ImVec2(0, 0), true);
    if (std::shared_ptr<FileEntry> dir = app->getFileSystemModule()->getEntry(m_currentDirectory))
    {
        drawAssetGrid(dir);
    }
    ImGui::EndChild();
    ImGui::End();
}

