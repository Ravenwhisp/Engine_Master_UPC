#pragma once
#include "Component.h"
#include <vector>

class CameraComponent;

class CameraSwitcherComponent final : public Component
{
public:
    CameraSwitcherComponent(UID id, GameObject* gameObject);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    bool init() override;
    void update() override;
    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;

private:
    void rebuildCameraList();
    void syncCurrentIndexWithDefaultCamera();
    void switchToNextCamera();

private:
    std::vector<CameraComponent*> m_cameras;
    int m_currentIndex = -1;
    bool m_wasSwitchKeyPressed = false;

    bool m_hasBuiltCameraList = false;
};