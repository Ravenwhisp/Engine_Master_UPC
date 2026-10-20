#include "Globals.h"
#include "SceneConfig.h"

#include "Application.h"
#include "ModuleScene.h"
#include "ModuleAssets.h"
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
    drawSSAOSettings();
    ImGui::Separator();
    drawVolumetricFogSettings();
    ImGui::Separator();
    drawPostProcessSettings();
    ImGui::Separator();
    drawMusicBanksSettings();
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

        ImGui::SameLine();

        if (ImGui::Button("Async Load"))
        {
            m_moduleScene->requestAsyncSceneLoad(m_loadSceneName);
        }

        ImGui::SameLine();

        if (ImGui::Button("Apply Async"))
        {
            m_moduleScene->requestSceneChange(m_loadSceneName);
        }

        if (m_moduleScene->isPendingSceneLoad())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Scene change pending...");
        }
        else if (m_moduleScene->isAsyncSceneReady())
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Async scene ready.");
        }
        else if (m_moduleScene->isAsyncSceneLoading())
        {
            ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Async scene loading...");
        }
        else
        {
            ImGui::TextDisabled("No scene load pending.");
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
                AssetId* data = static_cast<AssetId*>(payload->Data);
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

        ImGui::Separator();
        ImGui::Checkbox("Light Tile Debug View###LightTileDebugView", &light.tileDebugView);
        ImGui::TextDisabled("Colours each pixel by how many lights its tile has.");
    }
}

void SceneConfig::drawSSAOSettings()
{
    SSAOSettings& ssao = m_moduleScene->getScene()->getSSAOSettings();

    if (ImGui::CollapsingHeader("SSAO"))
    {
        ImGui::Checkbox("Enabled###SSAOEnabled", &ssao.enabled);
        ImGui::Checkbox("Blur Enabled###SSAOBlurEnabled", &ssao.blurEnabled);
        ImGui::Checkbox("Debug View###SSAODebugView", &ssao.debugView);

        ImGui::Separator();

        ImGui::DragFloat("Radius###SSAORadius", &ssao.radius, 0.01f, 0.01f, 5.0f);
        ImGui::DragFloat("Bias###SSAOBias", &ssao.bias, 0.0001f, 0.0f, 0.1f, "%.4f");
        ImGui::DragFloat("Strength###SSAOStrength", &ssao.strength, 0.01f, 0.0f, 8.0f);

        int sampleCount = static_cast<int>(ssao.sampleCount);

        if (ImGui::SliderInt("Sample Count###SSAOSampleCount", &sampleCount, 1, SSAO_KERNEL_SIZE))
        {
            sampleCount = std::clamp(sampleCount, 1, static_cast<int>(SSAO_KERNEL_SIZE));
            ssao.sampleCount = static_cast<uint32_t>(sampleCount);
        }

        ImGui::Separator();

        if (ImGui::Button("Reset Defaults###SSAOResetDefaults"))
        {
            ssao = SSAOSettings{};
        }
    }
}

void SceneConfig::drawVolumetricFogSettings()
{
    VolumetricFogSettings& fog = m_moduleScene->getScene()->getVolumetricFogSettings();

    auto drawTooltip = [](const char* text)
        {
            if (!ImGui::IsItemHovered()) return;

            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextWrapped("%s", text);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        };

    if (ImGui::CollapsingHeader("Volumetric Fog"))
    {
        ImGui::TextDisabled("General / Global Fog");
        ImGui::Checkbox("Enabled###VolFogEnabled", &fog.enabled);
        drawTooltip("Enables volumetric fog for this scene.");

        ImGui::Separator();

        ImGui::DragFloat("Density###VolFogDensity", &fog.density, 0.01f, 0.0f, 10.0f, "%.3f");
        drawTooltip("Uniform fog density, added to Height Fog. Higher values make the fog thicker.");

        ImGui::DragFloat("Scattering Coefficient###VolFogScattering", &fog.scatteringCoefficient, 0.001f, 0.0f, 1.0f, "%.4f");
        drawTooltip("How much light the fog scatters. Higher values brighten illuminated fog.");

        ImGui::DragFloat("Extinction Coefficient###VolFogExtinction", &fog.extinctionCoefficient, 0.001f, 0.0f, 1.0f, "%.4f");
        drawTooltip("How quickly fog blocks light. Must be at least as high as scattering.");

        ImGui::TextDisabled("Extinction must be greater than or equal to scattering.");

        ImGui::Separator();

        ImGui::SliderFloat("Anisotropy (g)###VolFogAnisotropy", &fog.anisotropy, -0.99f, 0.99f, "%.2f");
        drawTooltip("Scattering direction: 0 is uniform, positive is forward, negative is backward.");

        ImGui::DragFloat("Max Distance###VolFogMaxDistance", &fog.maxDistance, 1.0f, 1.0f, 1000.0f, "%.1f");
        drawTooltip("Fog coverage from the camera. Longer distances reduce depth detail.");

        fog.sanitize();

        // -----------------------------------------------------------------
        // Exponential Height Fog
        // -----------------------------------------------------------------

        ImGui::Separator();
        if (ImGui::TreeNodeEx("Exponential Height Fog###VolFogHeightSection", ImGuiTreeNodeFlags_SpanAvailWidth))
        {

            ImGui::Checkbox("Enabled###VolumetricFogHeightEnabled", &fog.heightFogEnabled);
            drawTooltip("Adds fog below Base Height that fades above it. Shadow sampling adds GPU cost.");

            if (fog.heightFogEnabled)
        {
                ImGui::DragFloat("Density###VolumetricFogHeightDensity", &fog.heightFogDensity, 0.01f, 0.0f, 10.0f, "%.3f");
                drawTooltip("Extra density at and below Base Height, added to global fog.");

                ImGui::DragFloat("Base Height###VolumetricFogHeightBase", &fog.heightFogBaseHeight, 0.1f, -10000.0f, 10000.0f, "%.2f");
                drawTooltip("World Y height where fog starts thinning. Density is constant below it.");

                ImGui::DragFloat("Height Falloff###VolumetricFogHeightFalloff", &fog.heightFogFalloff, 0.001f, 0.0f, 10.0f, "%.4f");
                drawTooltip("How quickly fog thins above Base Height. Higher values create a thinner layer.");
            }

            ImGui::TreePop();
        }

        // -----------------------------------------------------------------
        // Distance Fog
        // -----------------------------------------------------------------

        ImGui::Separator();
        if (ImGui::TreeNodeEx("Distance Fog###VolFogDistanceSection", ImGuiTreeNodeFlags_SpanAvailWidth))
        {

            ImGui::Checkbox("Enabled###VolumetricFogDistanceEnabled", &fog.distanceFogEnabled);
            drawTooltip("Adds a colored haze to hide distant areas.");

            if (fog.distanceFogEnabled)
        {
                ImGui::DragFloat("Start Distance###VolumetricFogDistanceStart", &fog.distanceFogStart, 1.0f, 0.0f, 10000.0f, "%.1f");
                drawTooltip("Camera distance where the haze begins.");

                ImGui::DragFloat("End Distance###VolumetricFogDistanceEnd", &fog.distanceFogEnd, 1.0f, 0.0f, 10000.0f, "%.1f");
                drawTooltip("Camera distance where the haze reaches Max Opacity.");

                ImGui::SliderFloat("Max Opacity###VolumetricFogDistanceMaxOpacity", &fog.distanceFogMaxOpacity, 0.0f, 1.0f, "%.2f");
                drawTooltip("Maximum haze coverage: 0 is transparent, 1 fully covers the background.");

                float fogColor[3] = { fog.distanceFogColorR, fog.distanceFogColorG, fog.distanceFogColorB };

                if (ImGui::ColorEdit3("Fog Color###VolumetricFogDistanceColor", fogColor))
                {
                    fog.distanceFogColorR = fogColor[0];
                    fog.distanceFogColorG = fogColor[1];
                    fog.distanceFogColorB = fogColor[2];
                }

                drawTooltip("Color blended into distant areas.");
            }

            ImGui::TreePop();
        }

        // -----------------------------------------------------------------
        // Animation
        // -----------------------------------------------------------------

        ImGui::Separator();
        if (ImGui::TreeNodeEx("Animation###VolFogAnimationSection", ImGuiTreeNodeFlags_SpanAvailWidth))
        {

            ImGui::Checkbox("Animate Density###VolumetricFogAnimateDensity", &fog.animateDensity);
            drawTooltip("Moves a noise pattern through the fog density. Adds GPU work.");

            if (fog.animateDensity)
        {
                ImGui::DragFloat("Noise Scale###VolumetricFogNoiseScale", &fog.noiseScale, 0.001f, 0.001f, 1.0f);
                drawTooltip("Lower values create large formations; higher values create finer patterns.");

                ImGui::SliderFloat("Noise Strength###VolumetricFogNoiseStrength", &fog.noiseStrength, 0.0f, 1.0f);
                drawTooltip("Density variation: 0 is uniform, 1 uses the full noise pattern.");

                float windDirection[3] = { fog.windDirectionX, fog.windDirectionY, fog.windDirectionZ };

                if (ImGui::DragFloat3("Wind Direction###VolumetricFogWindDirection", windDirection, 0.01f))
                {
                    fog.windDirectionX = windDirection[0];
                    fog.windDirectionY = windDirection[1];
                    fog.windDirectionZ = windDirection[2];
                }

                drawTooltip("Direction of fog movement in world space.");

                ImGui::DragFloat("Wind Speed###VolumetricFogWindSpeed", &fog.windSpeed, 0.01f, 0.0f, 100.0f);
                drawTooltip("Speed of the moving fog pattern.");
            }

            ImGui::TreePop();
        }

        ImGui::Separator();
        if (ImGui::TreeNodeEx("Lighting / Quality###VolFogQualitySection", ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            ImGui::SliderInt("Shadow Samples###VolFogShadowSamples", &fog.shadowSampleCount, 1, 32);
            drawTooltip("More samples reduce shadow bands but cost more GPU time. Used with Height Fog or the Shadow Sampling view.");
            ImGui::Checkbox("Optimized Shadow Filter###VolFogOptimizedShadows", &fog.optimizedShadowFilterEnabled);
            drawTooltip("Cheaper shadow filtering for fog. Disabling it can increase GPU cost.");
            ImGui::Checkbox("Point Light Culling###VolFogPointLightCulling", &fog.pointLightCullingEnabled);
            drawTooltip("Skips point lights that cannot reach the fog cell to save GPU time.");
            ImGui::TreePop();
        }

        // -----------------------------------------------------------------
        // Sampling
        // -----------------------------------------------------------------

        ImGui::Separator();
        if (ImGui::TreeNodeEx("Sampling / Smoothing###VolFogSamplingSection", ImGuiTreeNodeFlags_SpanAvailWidth))
        {

            ImGui::Checkbox("Sampling Jitter###VolumetricFogSamplingJitter", &fog.samplingJitterEnabled);
            drawTooltip("Offsets density samples to break up regular patterns. Can introduce noise.");

            if (fog.samplingJitterEnabled)
        {
                ImGui::Checkbox("Blue Noise Jitter###VolFogBlueNoise", &fog.blueNoiseJitterEnabled);
                drawTooltip("Uses blue noise instead of the original jitter pattern.");
                ImGui::SliderFloat("Jitter Strength###VolumetricFogSamplingJitterStrength", &fog.samplingJitterStrength, 0.0f, 1.0f, "%.2f");
                drawTooltip("Sample displacement: 0 is centered, 1 uses the full jitter range.");
            }

            ImGui::Checkbox("Fog Spatial Smoothing###VolFogSpatialSmoothing", &fog.spatialSmoothingEnabled);
            drawTooltip("Softens fog noise without frame history. Adds GPU cost to Apply.");
            if (fog.spatialSmoothingEnabled)
        {
                ImGui::SliderFloat("Smoothing Strength###VolFogSmoothingStrength", &fog.spatialSmoothingStrength, 0.0f, 1.0f, "%.2f");
                drawTooltip("Blends more strongly with nearby fog. High values soften small details.");
            }

            ImGui::TreePop();
        }

        // -----------------------------------------------------------------
        // Debug
        // -----------------------------------------------------------------

        ImGui::Separator();
        if (ImGui::TreeNodeEx("Debug###VolFogDebugSection", ImGuiTreeNodeFlags_SpanAvailWidth))
        {

            ImGui::Checkbox("Disable Point Lights (Fog Only)###VolFogDisablePointLights", &fog.debugDisablePointLights);
            drawTooltip("Excludes point lights from fog only, for debugging. Not saved with the scene.");
            ImGui::Checkbox("Disable Spot Lights (Fog Only)###VolFogDisableSpotLights", &fog.debugDisableSpotLights);
            drawTooltip("Excludes spot lights from fog only, for debugging. Not saved with the scene.");

            static const char* DEBUG_VIEW_NAMES[] = {
                "Final",
                "Medium / Extinction Slice",
                "Lighting Slice",
                "Lighting Slice - No Shadows",
                "Accumulated Scattering",
                "Transmittance",
                "Final - Shadow Sampling",
                "Shadow Visibility Slice",
                "Shadow Cascade Slice"
            };

            int debugView = static_cast<int>(fog.debugView);

            if (ImGui::Combo("Debug View###VolumetricFogDebugView", &debugView, DEBUG_VIEW_NAMES, IM_ARRAYSIZE(DEBUG_VIEW_NAMES)))
        {
                fog.debugView = static_cast<VolumetricFogDebugView>(debugView);
            }

            static const char* DEBUG_VIEW_TOOLTIPS[] = {
                "Final scene with fog.",
                "Light absorption in one fog layer.",
                "Local lighting in one fog layer, before accumulation.",
                "Local fog lighting with shadow queries disabled.",
                "Fog light accumulated toward the camera.",
                "Remaining scene visibility: white is clear, black is opaque.",
                "Final image using Shadow Samples. More samples cost more GPU time.",
                "Shadow visibility: black is shadow, white is lit, magenta is unavailable.",
                "Shadow cascade colors: red, green, blue, yellow. Magenta means no coverage."
            };
            drawTooltip(DEBUG_VIEW_TOOLTIPS[static_cast<int>(fog.debugView)]);

            if (fog.debugView == VolumetricFogDebugView::Medium ||
                fog.debugView == VolumetricFogDebugView::ShadowVisibility ||
                fog.debugView == VolumetricFogDebugView::ShadowCascades ||
                fog.debugView == VolumetricFogDebugView::Lighting ||
                fog.debugView == VolumetricFogDebugView::LightingNoShadows)
        {
                ImGui::SliderFloat("Debug Slice###VolumetricFogDebugSlice", &fog.debugSlice, 0.0f, 1.0f);
                drawTooltip("Layer to inspect: 0 is nearest to the camera, 1 is farthest.");
            }
            ImGui::TreePop();
        }

        fog.sanitize();
        ImGui::Separator();

        if (ImGui::Button("Reset Defaults###VolFogResetDefaults"))
        {
            fog = VolumetricFogSettings{};
        }

        drawTooltip("Resets every fog setting, including quality and debug options.");
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
        ImGui::DragFloat("Bloom Clamp###PPBloomClamp", &pp.bloomClamp, 0.1f, 0.5f, 8192.0f);
        ImGui::TextDisabled("Lower = tame blown-out discs from very bright lights.");

        ImGui::Separator();
        ImGui::Checkbox("Colour Grading (LUT)###PPLutEnabled", &pp.lutEnabled);
        ImGui::Text("LUT Asset");
        ImGui::SameLine();
        if (pp.lutAsset.isValid())
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Assigned (UID %llu)", pp.lutAsset.m_uid);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "None");
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
            {
                const UID droppedUID = *static_cast<const UID*>(payload->Data);
                std::unique_ptr<AssetId> resolved(app->getModuleAssets()->findReference(droppedUID));
                if (resolved && resolved->m_type == AssetType::LUT) pp.lutAsset = *resolved;
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear###PPLutClear")) pp.lutAsset = AssetId();
        ImGui::SliderFloat("LUT Strength###PPLutStrength", &pp.lutStrength, 0.0f, 1.0f);
        ImGui::TextDisabled("Drag an imported .CUBE LUT asset here.");

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
        ImGui::DragFloat("Depth Threshold (silhouette)###PPOutThresh", &pp.outlineThreshold, 0.001f, 0.001f, 0.5f, "%.3f");
        ImGui::DragFloat("Normal Threshold (creases)###PPOutNormalThresh", &pp.outlineNormalThreshold, 0.005f, 0.01f, 2.0f, "%.3f");
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
        ImGui::TextDisabled("Depth threshold catches silhouettes; normal threshold catches interior creases and stays stable while zooming.");
    }
}

void SceneConfig::drawMusicBanksSettings()
{
    const std::vector<std::string> loadedBanks = m_moduleScene->getScene()->getLoadedBankNames();
    std::vector<WwiseBank>& existingBanks = m_moduleMusic->getBankList();

    if (!ImGui::CollapsingHeader("Music Banks"))
    {
        return;
    }

    ImGui::Text("Scene assigned banks");
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
            const bool isAssigned = std::find(loadedBanks.begin(), loadedBanks.end(), bankName) != loadedBanks.end();
            const bool isRuntimeLoaded = bank.isLoaded();

            ImGui::PushID(bankName.c_str());

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(bankName.c_str());

            ImGui::TableSetColumnIndex(1);
            if (isRuntimeLoaded)
            {
                ImGui::TextDisabled(isAssigned ? "Loaded" : "Loaded (unassigned)");
            }
            else
            {
                ImGui::TextDisabled(isAssigned ? "Load failed" : "Unloaded");
            }

            ImGui::TableSetColumnIndex(2);

            if (isAssigned && isRuntimeLoaded)
            {
                if (ImGui::Button("Unload", ImVec2(-1.0f, 0.0f)))
                {
                    if (m_moduleMusic->unloadBank(bankName))
                    {
                        m_moduleScene->getScene()->removeLoadedBank(bankName);
                    }
                }
            }
            else if (isAssigned)
            {
                if (ImGui::Button("Retry", ImVec2(-1.0f, 0.0f)))
                {
                    m_moduleMusic->loadBank(bankName);
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
