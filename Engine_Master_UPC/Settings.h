#pragma once
#include "Globals.h"

// to check if fits with current camera (copied from another implementation)
struct CameraSettings {
    float panSpeed = 0.0025f;
    bool  panInvertX = false;
    bool  panInvertY = false;

    float orbitSpeed = 0.005f;
    bool  orbitInvertX = false;
    bool  orbitInvertY = true;

    float zoomWheelSpeed = 0.01f;
    float zoomDragSpeed = 0.01f;
    float minDistance = 0.5f;
    float maxDistance = 5000.0f;

    float flyMoveSpeed = 10.0f;
    float flyBoostMult = 4.0f;
    float flyRotSpeed = 0.0035f;
    bool  flyInvertX = false;
    bool  flyInvertY = true;
    float flyPitchClamp = 0.01f;
};

struct SceneEditorSettings {
    bool showGrid = true;
    bool showAxis = true;
    bool showGuizmo = true;
    bool showQuadTree = true;
};

struct SkyboxSettings
{
    bool enabled = true;
    char path[260] = "Assets/Textures/cubemap2.dds";
    bool dirty = false;
};

class Settings {
public:
    CameraSettings camera;
    SceneEditorSettings sceneEditor;
    SkyboxSettings skybox;

public:
    void loadSettings() {
        //to do, probably load.json on disk also can be bin more faster
    }

    void saveSettings() {
        //to do
    }
};
