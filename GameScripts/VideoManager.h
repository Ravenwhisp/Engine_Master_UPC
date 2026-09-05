#pragma once

#include "ScriptAPI.h"

class ComponentVideo;

class VideoManager : public Script
{
    DECLARE_SCRIPT(VideoManager)

public:
    explicit VideoManager(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

public:
    ComponentRef<Transform> m_videoObject;
    std::string m_sceneToLoad;

private:
    ComponentVideo* m_videoComponent = nullptr;
    bool m_started = false;
};
