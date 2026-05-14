#include "Globals.h"
#include "SceneConfig.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleNavigation.h"
#include "ModuleMusic.h"

#include "Scene.h"
#include "WwiseBank.h"

#include <algorithm>

SceneConfig::SceneConfig()
{
    m_moduleScene = app->getModuleScene();
    m_moduleMusic = app->getModuleMusic();
}

void SceneConfig::drawInternal()
{
    drawSaveSceneSettings();
    ImGui::Separator();
    drawLoadSceneSettings();
    ImGui::Separator();
    drawNavmeshSettings();
    ImGui::Separator();
    drawSkyBoxSettings();
    ImGui::Separator();
    drawLightSettings();
    ImGui::Separator();
    drawMusicBanksSettings();
}

void SceneConfig::drawSaveSceneSettings()
{
    if (ImGui::CollapsingHeader("Save Scene"))
    {
        static char saveSceneBuffer[256];
        strcpy_s(saveSceneBuffer, m_saveSceneName.c_str());

        if (ImGui::InputText("Scene Name##Save", saveSceneBuffer, IM_ARRAYSIZE(saveSceneBuffer)))
        {
            m_saveSceneName = saveSceneBuffer;
        }

        if (ImGui::Button("Save"))
        {
            const bool blank = (m_saveSceneName.find_first_not_of(" \t\n\r") == std::string::npos);

            if (blank)
            {
                DEBUG_WARN("Cannot save scene: name is empty.");
            }
            else
            {
                m_moduleScene->getScene()->setName(m_saveSceneName.c_str());
                m_moduleScene->saveScene();
            }
        }
    }
}

void SceneConfig::drawLoadSceneSettings()
{
    if (ImGui::CollapsingHeader("Load Scene"))
    {
        static char loadSceneBuffer[256];
        strcpy_s(loadSceneBuffer, m_loadSceneName.c_str());

        if (ImGui::InputText("Scene Name##Load", loadSceneBuffer, IM_ARRAYSIZE(loadSceneBuffer)))
        {
            m_loadSceneName = loadSceneBuffer;
        }

        if (ImGui::Button("Load"))
        {
            m_moduleScene->requestSceneChange(m_loadSceneName);
        }
    }
}

void SceneConfig::drawNavmeshSettings()
{
    if (ImGui::CollapsingHeader("Navmesh"))
    {
        ModuleNavigation* nav = app->getModuleNavigation();

        if (ImGui::Button("Bake NavMesh"))
        {
            if (app->getModuleNavigation()->buildNavMeshForCurrentScene())
            {
                DEBUG_LOG("NavMesh bake SUCCESS\n");
            }
            else
            {
                DEBUG_ERROR("NavMesh bake FAILED\n");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Navmesh"))
        {
            const char* sceneName = m_moduleScene->getScene()->getName();
            const bool ok = app->getModuleNavigation()->loadNavMeshForScene(sceneName);

            if (ok)
            {
                DEBUG_LOG("Navmesh loaded successfully.");
            }
            else
            {
                DEBUG_WARN("Navmesh for scene '%s' not found.", sceneName);
            }
        }

        bool show = nav->getDrawNavMesh();

        if (ImGui::Checkbox("Show NavMesh###ShowNavMesh", &show))
        {
            nav->setDrawNavMesh(show);
        }

        ImGui::Separator();
        ImGui::DragFloat("Cell Size", &nav->getSettings().cellSize, 0.01f, 0.05f, 1.0f);
        ImGui::DragFloat("Cell Height", &nav->getSettings().cellHeight, 0.01f, 0.05f, 1.0f);
        ImGui::DragFloat("Agent Height", &nav->getSettings().agentHeight, 0.05f, 0.5f, 5.0f);
        ImGui::DragFloat("Agent Radius", &nav->getSettings().agentRadius, 0.01f, 0.1f, 2.0f);
        ImGui::DragFloat("Max Climb", &nav->getSettings().agentMaxClimb, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Max Slope", &nav->getSettings().agentMaxSlope, 1.0f, 0.0f, 60.0f);
    }
}

void SceneConfig::drawSkyBoxSettings()
{
    auto& skyboxSettings = m_moduleScene->getScene()->getSkyBoxSettings();

    if (ImGui::CollapsingHeader("SkyBox"))
    {
        if (ImGui::Checkbox("Enabled###SkyEnabled", &skyboxSettings.enabled))
        {
            m_skyboxDirty = true;
        }

        ImGui::Button("Drop Cubemap Here###SkyDrop");

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
            {
                const MD5Hash* data = static_cast<const MD5Hash*>(payload->Data);
                skyboxSettings.cubemapAssetId = *data;
                m_skyboxDirty = true;
            }

            ImGui::EndDragDropTarget();
        }

        if (ImGui::Button("Apply###SkyApply") && m_skyboxDirty)
        {
            m_skyboxDirty = false;
        }
    }
}

void SceneConfig::drawLightSettings()
{
    SceneLightingSettings& light = m_moduleScene->getScene()->getLightingSettings();

    if (ImGui::CollapsingHeader("Lighting"))
    {
        ImGui::ColorEdit3("Ambient Color###AmbientColor", &light.ambientColor.x);
        ImGui::DragFloat("Ambient Intensity###AmbientIntensity", &light.ambientIntensity, 0.01f, 0.0f, 50.0f);
    }
}

void SceneConfig::drawMusicBanksSettings()
{
    const std::vector<std::string> loadedBanks = m_moduleScene->getScene()->getLoadedBanks();
    std::vector<WwiseBank>& existingBanks = m_moduleMusic->getBankList();

    if (!ImGui::CollapsingHeader("Music Banks"))
    {
        return;
    }

    ImGui::Text("Scene loaded banks");
    ImGui::SameLine();
    ImGui::TextDisabled("(%zu / %zu)", loadedBanks.size(), existingBanks.size());

    ImGui::Separator();

    if (existingBanks.empty())
    {
        ImGui::TextDisabled("No music banks found");
        return;
    }

    if (ImGui::BeginTable("MusicBanksTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Bank", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableHeadersRow();

        for (WwiseBank& bank : existingBanks)
        {
            const std::string bankName = bank.getName();
            const bool isLoaded = std::find(loadedBanks.begin(), loadedBanks.end(), bankName) != loadedBanks.end();

            ImGui::PushID(bankName.c_str());

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(bankName.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextDisabled(isLoaded ? "Loaded" : "Unloaded");

            ImGui::TableSetColumnIndex(2);

            if (isLoaded)
            {
                if (ImGui::Button("Unload", ImVec2(-1.0f, 0.0f)))
                {
                    bank.unload();
                    m_moduleScene->getScene()->removeLoadedBank(bankName);
                }
            }
            else
            {
                if (ImGui::Button("Load", ImVec2(-1.0f, 0.0f)))
                {
                    if (bank.load())
                    {
                        m_moduleScene->getScene()->addLoadedBank(bankName);
                    }
                }
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}
