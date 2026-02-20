#pragma once
#include "EditorWindow.h"

class Settings;

class EditorSettings : public EditorWindow
{
public:
    EditorSettings();
    ~EditorSettings() override = default;

    const char* getWindowName() const override { return "Editor Settings"; }
    void render() override;

private:
    void drawCameraSettings();
    void drawSceneSettings();

private:
    Settings* m_settings = nullptr;
};
