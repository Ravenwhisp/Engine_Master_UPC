#include "Globals.h"
#include "FileDialog.h"

#include "Application.h"
#include "FileSystemModule.h"
#include "AssetsModule.h"

void FileDialog::drawDirectoryTree(const std::shared_ptr<FileEntry> entry)
{
    std::string nodeName = entry->path.filename().string();

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

    ImGui::Columns(columnCount, nullptr, false);

    for (auto& asset : directory->children)
    {
        if (asset == nullptr) 
        {
            continue;
        }

        ImGui::PushID(asset->path.string().c_str());

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
                AssetMetadata metadata;
                std::filesystem::path metaPath = asset->path;
                metaPath += METADATA_EXTENSION;
                AssetMetadata::loadMetaFile(metaPath, metadata);

                ImGui::SetDragDropPayload("ASSET", &metadata.uid, sizeof(UID));
                ImGui::Text("Dragging %s", asset->path.lexically_normal().c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginPopupContextItem("ItemContext"))
            {
                ImGui::Text("Options");
                ImGui::Separator();

                Importer* importer = app->getFileSystemModule()->findImporter(asset->path);
                bool canImport = importer != nullptr;
                if (ImGui::MenuItem("Import", nullptr, false, canImport)) 
                {
                    app->getAssetModule()->import(asset->path);
                }
                ImGui::EndPopup();
            }
        }

        ImGui::TextWrapped("%s", asset->path.filename().string().c_str());

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

