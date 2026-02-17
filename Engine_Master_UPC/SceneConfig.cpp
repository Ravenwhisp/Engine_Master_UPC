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

    ImGui::Spacing();
    ImGui::SeparatorText("Skybox");

    static char skyboxBuffer[256];
    strcpy_s(skyboxBuffer, m_skybox_path.c_str());

    if (ImGui::InputText("Skybox Path", skyboxBuffer, IM_ARRAYSIZE(skyboxBuffer)))
    {
        m_skybox_path = skyboxBuffer;
    }

    ImGui::SameLine();

    if (ImGui::Button("Reload"))
    {
        LOG("PULSADO\n");
    }

    ImGui::End();
}

