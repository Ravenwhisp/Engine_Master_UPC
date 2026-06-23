#include "Globals.h"
#include "SceneConfig.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleAssets.h"
#include "ModuleNavigation.h"
#include "ModuleMusic.h"

#include "Scene.h"
#include "WwiseBank.h"

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
    drawPostProcessSettings();
    ImGui::Separator();
    drawMusicBanksSettings();
    ImGui::Separator();
}

void SceneConfig::drawSaveSceneSettings()
{
    if (ImGui::CollapsingHeader("Save Scene"))
    {
        if (ImGui::Button("Save"))
        {
            m_moduleScene->saveScene();
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


        /*ImGui::Button("Load");
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
            {
                UID* ref = static_cast<UID*>(payload->Data);
                AssetReference assetRef(*ref, INVALID_ASSET_ID, AssetType::SCENE);
                auto scene = app->getModuleAssets()->load<Scene>(assetRef);
                if (scene)
                {
                    m_moduleScene->requestSceneChange(scene);
                }
            }
            ImGui::EndDragDropTarget();
        }*/
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
        ImGui::DragFloat("Region Min Size", &nav->getSettings().regionMinSize, 0.5f, 0.0f, 150.0f);
        ImGui::DragFloat("Region Merge Size", &nav->getSettings().regionMergeSize, 0.5f, 0.0f, 200.0f);
        ImGui::DragFloat("Edge Max Len", &nav->getSettings().edgeMaxLen, 0.5f, 0.0f, 64.0f);
        ImGui::DragFloat("Edge Max Error", &nav->getSettings().edgeMaxError, 0.1f, 0.1f, 10.0f);
        ImGui::DragInt("Verts Per Poly", &nav->getSettings().vertsPerPoly, 1, 3, 12);
        ImGui::DragFloat("Detail Sample Dist", &nav->getSettings().detailSampleDist, 0.1f, 0.0f, 16.0f);
        ImGui::DragFloat("Detail Sample Max Error", &nav->getSettings().detailSampleMaxError, 0.1f, 0.0f, 16.0f);
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
                AssetReference* data = static_cast<AssetReference*>(payload->Data);
                if (auto cubemapRef = app->getModuleAssets()->findReference(data->m_uid))
                {
                    skyboxSettings.cubemapAssetId = *cubemapRef;
                    m_skyboxDirty = true;
                }
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

void SceneConfig::drawPostProcessSettings()
{
    PostProcessSettings& pp = m_moduleScene->getScene()->getPostProcessSettings();

    if (ImGui::CollapsingHeader("Post Process"))
    {
        ImGui::DragFloat("Exposure (EV)###PPExposure", &pp.exposure, 0.05f, -10.0f, 10.0f);
        ImGui::TextDisabled("One stop (EV +1) doubles luminance.");

        ImGui::Separator();
        ImGui::Checkbox("Bloom###PPBloomEnabled", &pp.bloomEnabled);
        ImGui::DragFloat("Bloom Threshold###PPBloomThreshold", &pp.bloomThreshold, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Bloom Intensity###PPBloomIntensity", &pp.bloomIntensity, 0.01f, 0.0f, 5.0f);

        ImGui::Separator();
        ImGui::Checkbox("Colour Grading (LUT)###PPLutEnabled", &pp.lutEnabled);

        char lutBuffer[260];
        strcpy_s(lutBuffer, pp.lutPath.c_str());
        if (ImGui::InputText(".CUBE Path###PPLutPath", lutBuffer, IM_ARRAYSIZE(lutBuffer)))
        {
            pp.lutPath = lutBuffer;
        }
        ImGui::TextDisabled("Path to a .CUBE LUT (relative to the working directory).");

        ImGui::Separator();
        ImGui::Checkbox("Chromatic Aberration###PPCAEnabled", &pp.chromaticAberrationEnabled);
        ImGui::DragFloat("CA Strength###PPCAStrength", &pp.chromaticAberrationStrength, 0.05f, 0.0f, 10.0f);

        ImGui::Separator();
        ImGui::Checkbox("Heartbeat (Damage FX)###PPHeartbeat", &pp.heartbeatEnabled);
        ImGui::DragFloat("Health Threshold###PPHealthThreshold", &pp.healthThreshold, 0.01f, 0.0f, 1.0f);
        ImGui::SliderFloat("Health (test)###PPHealth", &pp.health, 0.0f, 1.0f);
        ImGui::SliderFloat("Separation (test)###PPSeparation", &pp.separation, 0.0f, 1.0f);
        ImGui::TextDisabled("Health/Separation are normally driven by gameplay.");

        ImGui::Separator();
        ImGui::Checkbox("Death Fade (test)###PPDeath", &pp.deathFadeActive);
        ImGui::DragFloat("Grey Duration (s)###PPDeathGrey", &pp.deathGreyDuration, 0.05f, 0.1f, 10.0f);
        ImGui::DragFloat("Black Duration (s)###PPDeathBlack", &pp.deathBlackDuration, 0.05f, 0.1f, 10.0f);
        ImGui::TextDisabled("Triggered by gameplay when all players are down.");

        ImGui::Separator();
        ImGui::Checkbox("Outline (Ink)###PPOutline", &pp.outlineEnabled);
        ImGui::DragFloat("Thickness (px)###PPOutThick", &pp.outlineThickness, 0.05f, 0.5f, 6.0f);
        ImGui::DragFloat("Threshold###PPOutThresh", &pp.outlineThreshold, 0.001f, 0.001f, 0.5f, "%.3f");
        ImGui::DragFloat("Intensity###PPOutIntensity", &pp.outlineIntensity, 0.01f, 0.0f, 1.0f);
        float ink[3] = { pp.outlineColorR, pp.outlineColorG, pp.outlineColorB };
        if (ImGui::ColorEdit3("Ink Colour###PPOutColor", ink))
        {
            pp.outlineColorR = ink[0];
            pp.outlineColorG = ink[1];
            pp.outlineColorB = ink[2];
        }
        ImGui::DragFloat("Wobble###PPOutWobble", &pp.outlineWobble, 0.05f, 0.0f, 5.0f);
        ImGui::DragFloat("Noise Scale###PPOutNoise", &pp.outlineNoiseScale, 1.0f, 1.0f, 400.0f);
        ImGui::DragFloat("Break-up###PPOutBreakup", &pp.outlineBreakup, 0.01f, 0.0f, 1.0f);
        ImGui::TextDisabled("Depth-based; threshold is scene-dependent - tune to taste.");
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
                    if (m_moduleMusic->unloadBank(bankName))
                    {
                        m_moduleScene->getScene()->removeLoadedBank(bankName);
                    }
                }
            }
            else
            {
                if (ImGui::Button("Load", ImVec2(-1.0f, 0.0f)))
                {
                    if (m_moduleMusic->loadBank(bankName))
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
