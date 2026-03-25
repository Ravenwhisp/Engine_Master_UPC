#pragma once
#include "EditorWindow.h"

class Settings;

class WindowEditorSettings : public EditorWindow
{
public:
    WindowEditorSettings();
    ~WindowEditorSettings() override = default;

    const char* getWindowName() const override { return "Editor Settings"; }
    void drawInternal() override;

private:
    void drawEngineInformation();
    void drawCameraSettings();
    void drawSceneSettings();
    void drawFrustumCullingSettings();

private:
    Settings* m_settings = nullptr;
};
