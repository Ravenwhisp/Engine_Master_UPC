#pragma once

#define COMPONENT_TYPE_LIST \
    X(TRANSFORM)            \
    X(MODEL)                \
    X(LIGHT)                \
    X(SCRIPT)               \
    X(CAMERA)               \
    X(TRANSFORM2D)          \
    X(CANVAS)               \
    X(UIIMAGE)              \
    X(UITEXT)               \
    X(UIBUTTON)             \
    X(NAVIGATION_AGENT)     \
    X(WAYPOINT_PATH)        \
    X(CHANGE_SCENE)         \
    X(EXIT_APPLICATION)     \
    X(NAVMESH_WALK)         

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
