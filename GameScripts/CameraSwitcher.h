#pragma once

#include "ScriptAPI.h"

#include <vector>

class CameraSwitcher : public Script
{
    DECLARE_SCRIPT(CameraSwitcher)

public:
    explicit CameraSwitcher(GameObject* owner);

    void Start() override;
    void Update() override;
    void onAfterReferencesFixed() override;

private:
    void rebuildCameraList();
    void syncCurrentIndexWithDefaultCamera();
    void switchToNextCamera();

private:
    std::vector<GameObject*> m_cameras;
    int m_currentIndex = -1;
    bool m_wasSwitchKeyPressed = false;
};