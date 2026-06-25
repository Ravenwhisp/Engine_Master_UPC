#pragma once

#include "AssetReference.h"

#ifdef GAME_RELEASE
constexpr bool DEFAULT_DEBUG = false;
#else
constexpr bool DEFAULT_DEBUG = true;
#endif

struct EngineInformation
{
    std::string version = "alpha-v0.10";
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

struct WindowSceneEditorSettings
{
    bool showGrid = DEFAULT_DEBUG;
    bool showAxis = DEFAULT_DEBUG;
    bool showGuizmo = DEFAULT_DEBUG;
    bool showStaticQuadTree = false;
    bool showDynamicQuadTree = false;
    bool showModelBoundingBoxes = false;
    bool showNavPath = false;
    bool showLightComponent = false;
    bool showCameraFrustum = false;
};

struct FrustumCullingSettings
{
    bool enabled = false;
    float quadtreeXExtraSize = 10.0f;
    float quadtreeZExtraSize = 10.0f;
};

struct DebugGame
{
    bool showFPS = false;
    bool showFrametime = false;
    bool showTrianglesNumber = false;
    bool showMeshNumber = false;
    bool showScriptDebug = false;
};

struct RimErosionSettings
{
    bool enabled = false;
    AssetReference brushTextureAssetId{};
    float rimThreshold = 0.3f;
    float rimSoftness = 0.2f;
    float erosionIntensity = 1.0f;
    float brushScale = 10.0f;
    float brushOffsetX = 0.0f;
    float brushOffsetY = 0.0f;
    float displacementAmount = 0.08f;
    float erosionColor[3] = { 0.0f, 0.0f, 0.0f };
    float preserveSilhouette = 0.3f;
    bool debugRimMask = false;
    float paintColor1[3] = { 0.3f, 0.5f, 0.8f };
    float paintColor2[3] = { 0.9f, 0.7f, 0.3f };
    float brushNormalStrength = 1.5f;
    float curvatureScale = 100.0f;
    float toonSharpness = 0.02f;
};

class Settings
{
public:
    EngineInformation engine;
    CameraSettings camera;
    WindowSceneEditorSettings sceneEditor;
    FrustumCullingSettings frustumCulling;
    DebugGame debugGame;
    RimErosionSettings rimErosion;

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
        return debugGame.showFPS || debugGame.showFrametime || debugGame.showTrianglesNumber || debugGame.showMeshNumber  || debugGame.showScriptDebug;
    }
};
