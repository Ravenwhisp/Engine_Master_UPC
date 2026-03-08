#pragma once
#include "Globals.h"

#ifdef GAME_RELEASE
constexpr bool DEFAULT_DEBUG = false;
#else
constexpr bool DEFAULT_DEBUG = true;
#endif
struct EngineInformation
{
    std::string version = "alpha-v0.9";
};

struct CameraSettings
{
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

struct SceneEditorSettings
{
    bool showGrid = DEFAULT_DEBUG;
    bool showAxis = DEFAULT_DEBUG;
    bool showGuizmo = DEFAULT_DEBUG;
    bool showQuadTree = false;
    bool showModelBoundingBoxes = false;
    bool showNavPath = true;
};

struct FrustumCullingSettings
{
    bool debugFrustumCulling = !DEFAULT_DEBUG;
    float quadtreeXExtraSize = 10.0f;
    float quadtreeZExtraSize = 10.0f;
};

struct DebugGame
{
    bool showFPS = false;
    bool showFrametime = false;
    bool showTrianglesNumber = false;
};

class Settings
{
public:
    EngineInformation engine;
    CameraSettings camera;
    SceneEditorSettings sceneEditor;
    FrustumCullingSettings frustumCulling;
    DebugGame debugGame;

public:
    void loadSettings()
    {
        //to do, probably load.json on disk also can be bin more faster
    }

    void saveSettings()
    {
        //to do
    }

    bool hasDebugInformationEnabled() {
        return debugGame.showFPS || debugGame.showFrametime || debugGame.showTrianglesNumber;
    }
};
