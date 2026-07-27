#include "Globals.h"
#include "WindowDataContainerManager.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "AssetsDictionary.h"
#include "Extensions.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

WindowDataContainerManager::WindowDataContainerManager()
{
}

void WindowDataContainerManager::scanFiles()
{
    m_entries.clear();
    m_selected = -1;

    const std::filesystem::path root(ASSETS_FOLDER);
    if (!std::filesystem::exists(root))
        return;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() != DATA_CONTAINER_EXTENSION)
            continue;

        Entry e;
        e.path = entry.path().lexically_normal();
        e.displayName = e.path.stem().string();

        FILE* fp = std::fopen(e.path.string().c_str(), "rb");
        if (fp)
        {
            char readBuffer[4096];
            rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
            rapidjson::Document doc;
            doc.ParseStream(is);
            std::fclose(fp);

            if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("_typeName") && doc["_typeName"].IsString())
            {
                e.typeName = doc["_typeName"].GetString();
            }
        }

        if (e.typeName.empty())
            e.typeName = "DataContainer";

        m_entries.push_back(std::move(e));
    }
}

void WindowDataContainerManager::ensureLoaded(int i)
{
    if (i < 0 || i >= static_cast<int>(m_entries.size()))
        return;
    Entry& e = m_entries[i];
    if (e.asset)
        return;

    ModuleAssets* ma = app->getModuleAssets();
    const UID uid = ma->getIndex().findUID(e.path);
    if (!isValidUID(uid))
        return;

    AssetId* ref = ma->findReference(uid);
    if (!ref)
        return;

    e.asset = ma->load<DataContainer>(*ref);
}

void WindowDataContainerManager::drawList()
{
    ImGui::Text("Data Containers (%zu)", m_entries.size());
    ImGui::Separator();

    if (ImGui::BeginChild("DcList", ImVec2(0, 0), true))
    {
        for (int i = 0; i < static_cast<int>(m_entries.size()); ++i)
        {
            const Entry& e = m_entries[i];

            if (m_filter[0] != '\0')
            {
                std::string lowerFilter = m_filter;
                std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
                std::string lowerDisplay = e.displayName;
                std::transform(lowerDisplay.begin(), lowerDisplay.end(), lowerDisplay.begin(), ::tolower);
                std::string lowerType = e.typeName;
                std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
                if (lowerDisplay.find(lowerFilter) == std::string::npos &&
                    lowerType.find(lowerFilter) == std::string::npos)
                {
                    continue;
                }
            }

            ImGui::PushID(i);

            const bool isSelected = (i == m_selected);
            if (ImGui::Selectable("", isSelected, ImGuiSelectableFlags_AllowDoubleClick))
            {
                m_selected = i;
                ensureLoaded(i);
            }
            ImGui::SameLine();

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[%s]", e.typeName.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(e.displayName.c_str());

            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

void WindowDataContainerManager::drawDetail()
{
    if (m_selected < 0 || m_selected >= static_cast<int>(m_entries.size()))
    {
        ImGui::TextDisabled("Select a Data Container from the list.");
        return;
    }

    Entry& e = m_entries[m_selected];
    ensureLoaded(m_selected);

    if (!e.asset)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Failed to load '%s'.", e.displayName.c_str());
        return;
    }

    if (ImGui::BeginChild("DcDetail", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 4), true))
    {
        e.asset->drawUI();
    }
    ImGui::EndChild();

}

void WindowDataContainerManager::drawInternal()
{
    if (m_needsScan)
    {
        scanFiles();
        m_needsScan = false;
    }

    if (ImGui::Button("Refresh"))
    {
        m_entries.clear();
        m_selected = -1;
        scanFiles();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("|");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##filter", "Filter...", m_filter, sizeof(m_filter));

    ImGui::Separator();

    ImGui::Columns(2, "DcSplit", false);
    ImGui::SetColumnWidth(0, 300.0f);

    if (ImGui::BeginChild("LeftPanel", ImVec2(0, 0), true))
    {
        drawList();
    }
    ImGui::EndChild();

    ImGui::NextColumn();

    if (ImGui::BeginChild("RightPanel", ImVec2(0, 0), true))
    {
        drawDetail();
    }
    ImGui::EndChild();

    ImGui::Columns(1);
}
