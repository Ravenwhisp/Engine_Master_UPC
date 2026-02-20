#include "Globals.h"
#include "SceneConfig.h"

#include "Application.h"
#include "SceneModule.h"

SceneConfig::SceneConfig()
{
	m_sceneModule = app->getSceneModule();
}

void SceneConfig::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr()))
    {
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Scene");

    // Scene Name input
    static char sceneBuffer[256];
    strcpy_s(sceneBuffer, m_sceneName.c_str());

    if (ImGui::InputText("Scene Name", sceneBuffer, IM_ARRAYSIZE(sceneBuffer)))
    {
        m_sceneName = sceneBuffer;
    }

    ImGui::SameLine();

    if (ImGui::Button("Save"))
    {
		m_sceneModule->setName(m_sceneName.c_str());
        m_sceneModule->saveScene();
    }

    ImGui::SameLine();

    if (ImGui::Button("Load"))
    {
        m_sceneModule->setName(m_sceneName.c_str());
        m_sceneModule->loadScene();
    }

    ImGui::Separator();
    drawSkyboxSettings();

    ImGui::Separator();
    drawLightSettings();

    ImGui::End();

}

void SceneConfig::drawSkyboxSettings() {
    auto& skyboxSettings = m_sceneModule->getSkyboxSettings();

    if (ImGui::CollapsingHeader("Skybox")) {

        if (ImGui::Checkbox("Enabled###SkyEnabled", &skyboxSettings.enabled))
        {
            m_skyboxDirty = true;
        }

        if (ImGui::InputText("Cubemap Path###SkyPath", skyboxSettings.path, IM_ARRAYSIZE(skyboxSettings.path)))
        {
            m_skyboxDirty = true;
        }

        if (ImGui::Button("Apply###SkyApply") && m_skyboxDirty)
        {
            if (m_sceneModule->applySkyboxToRenderer())
            {
                m_skyboxDirty = false;
            }
        }
    }
}

void SceneConfig::drawLightSettings()
{
    auto& light = m_sceneModule->GetLightingSettings();

    if (ImGui::CollapsingHeader("Lighting"))
    {
        ImGui::ColorEdit3("Ambient Color###AmbientColor", &light.ambientColor.x);

        ImGui::DragFloat("Ambient Intensity###AmbientIntensity", &light.ambientIntensity, 0.01f, 0.0f, 50.0f);
    }
}
