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
    
    /*ImGui::SeparatorText("Scene");

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
    }*/

    drawSaveSceneSettings();

    ImGui::Separator();

    drawLoadSceneSettings();

    ImGui::Separator();

    drawSkyboxSettings();

    ImGui::Separator();

    drawLightSettings();

    ImGui::End();

}

void SceneConfig::drawSaveSceneSettings() {
    if (ImGui::CollapsingHeader("Save Scene")) {

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
                LOG("Cannot save scene: name is empty.");
            }
            else
            {
                m_sceneModule->setName(m_saveSceneName.c_str());
                m_sceneModule->saveScene();
            }
        }
    }
}


void SceneConfig::drawLoadSceneSettings() {
    if (ImGui::CollapsingHeader("Load Scene")) {
        static char loadSceneBuffer[256];
        strcpy_s(loadSceneBuffer, m_loadSceneName.c_str());

        if (ImGui::InputText("Scene Name##Load", loadSceneBuffer, IM_ARRAYSIZE(loadSceneBuffer)))
        {
            m_loadSceneName = loadSceneBuffer;
        }

        if (ImGui::Button("Load"))
        {
            if (!m_sceneModule->loadScene(m_loadSceneName))
            {
                LOG("Scene '%s' doesn't exist.", m_loadSceneName.c_str());
            }
        }
    }
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
