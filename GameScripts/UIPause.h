#pragma once

#include "ScriptAPI.h"
#include <string>
#include <vector>

class PlayerController;

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

    FieldList getExposedFields() const override;
    ScriptMethodList getExposedMethods() const override;

public:
    ComponentRef<Transform> m_pausePanel;
    ComponentRef<Transform> m_controlsPanel;

private:
    bool m_isPause = false;
    bool m_isControlsOpen = false;

    std::vector<PlayerController*> m_playerControllers;

    void SetPauseState(bool isPaused);
    void findPlayerControllers();
    void setPlayersGameplayInputLocked(bool locked);
};
