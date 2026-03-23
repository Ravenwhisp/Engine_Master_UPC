#pragma once

#define COMPONENT_TYPE_LIST \
    X(TRANSFORM)            \
    X(MODEL)                \
    X(LIGHT)                \
    X(PLAYER_WALK)          \
    X(CAMERA)               \
    X(TRANSFORM2D)          \
    X(CANVAS)               \
    X(UIIMAGE)              \
    X(UITEXT)               \
    X(UIBUTTON)             \
    X(CAMERA_FOLLOW)        \
    X(CHANGE_SCENE)         \
    X(EXIT_APPLICATION)     \
    X(CAMERA_SWITCHER)      \
    X(CHANGE_SCENE_ON_TRIGGER) \
    X(NAVMESH_WALK)         \
    X(NAVIGATION_AGENT)     \
    X(WAYPOINT_PATH)        \
    X(SCRIPT)               \
    X(UISLIDER)

enum class ComponentType
{
#define X(name) name,
    COMPONENT_TYPE_LIST
#undef X
    COUNT
};

inline const char* ComponentTypeToString(ComponentType type)
{
    switch (type)
    {
#define X(name) case ComponentType::name: return #name;
        COMPONENT_TYPE_LIST
#undef X
    default:
        return "Unknown";
    }
}
