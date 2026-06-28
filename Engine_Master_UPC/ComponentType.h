#pragma once

#define COMPONENT_TYPE_LIST(X) \
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
    X(ANIMATION)            \
    X(UISLIDER)             \
    X(TRIGGER)              \
    X(NAV_MODIFIER_VOLUME)  \
    X(NAV_RUNTIME_BLOCKER)  \
    X(PARTICLE_SYSTEM)      \
    X(UISHEET)              \
    X(SOUND_LISTENER)       \
    X(SOUND_SOURCE)         \
    X(PREFAB_INSTANCE)      \
    X(PLAYER_RENDER_BUFFER)

#define COMP_ENUM(name) name,
#define COMP_SWITCH(name) case ComponentType::name: return #name;
#define COMP_IF(name) if (std::strcmp(s, #name) == 0) return ComponentType::name;

enum class ComponentType
{
    COMPONENT_TYPE_LIST(COMP_ENUM)
    COUNT
};

inline const char* ComponentTypeToString(ComponentType type)
{
    switch (type)
    {
        COMPONENT_TYPE_LIST(COMP_SWITCH)
    default:
        return "Unknown";
    }
}

inline ComponentType StringToComponentType(const char* s)
{
    COMPONENT_TYPE_LIST(COMP_IF)
    return ComponentType::TRANSFORM;
}

inline const char* ComponentTypeToStringU32(uint32_t v) { return ComponentTypeToString(static_cast<ComponentType>(v)); }
inline uint32_t StringToComponentTypeU32(const char* s) { return static_cast<uint32_t>(StringToComponentType(s)); }
