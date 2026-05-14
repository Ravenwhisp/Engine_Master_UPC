#pragma once

#include "ScriptAPI.h"
#include <string>

class UIPause : public Script
{
    DECLARE_SCRIPT(UIPause)

public:
    explicit UIPause(GameObject* owner);

    void Start() override;
    void Update() override;

    void Resume();
    void ChangeScene(const std::string& sceneName);
    void ToggleControls(bool isOpen);

    ScriptFieldList getExposedFields() const override;
    ScriptMethodList getExposedMethods() const override;

public:
    ScriptComponentRef<Transform> m_pausePanel;
    ScriptComponentRef<Transform> m_controlsPanel;

private:
    bool m_isPause = false;
    bool m_isControlsOpen = false;

    void SetPauseState(bool isPaused);
};
