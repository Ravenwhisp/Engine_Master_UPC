#pragma once
#include "EditorWindow.h"

#include <array>

class Settings;

class WindowEditorSettings : public EditorWindow
{

public:

    WindowEditorSettings();
    ~WindowEditorSettings() override = default;

    const char* getWindowName() const override
    {
        return "Editor Settings";
    }

    void drawInternal() override;

private:
    void drawEngineInformation();
    void drawCameraSettings();
    void drawSceneSettings();
    void drawFrustumCullingSettings();
    void drawScriptsSettings();
    void drawRimErosionSettings();
    void drawScriptReloadModal();

private:
    Settings* m_settings = nullptr;

    std::array<char, 512> m_scriptProjectPathBuffer = {};
    std::array<char, 512> m_scriptSolutionDirBuffer = {};
    std::array<char, 64> m_scriptConfigurationBuffer = {};
    std::array<char, 64> m_scriptPlatformBuffer = {};
    bool m_scriptBuildSettingsSynced = false;
};
