#pragma once

#define LAYER_LIST(X) \
    X(DEFAULT)     \
    X(UI)          \
    X(ENVIRONMENT) \
    X(NAVMESH)     \
    X(PLAYER)      \
    X(ENEMY)       \
    X(PROJECTILE)  \
    X(BREAKABLE)   \
    X(PICKUP)

#define LAYER_ENUM(name) name,
#define LAYER_SWITCH(name) case Layer::name: return #name;
#define LAYER_IF(name) if (std::strcmp(string, #name) == 0) return Layer::name;

enum class Layer {
    LAYER_LIST(LAYER_ENUM)
    COUNT
};

inline const char* LayerToString(Layer layer)
{
    switch (layer)
    {
        LAYER_LIST(LAYER_SWITCH)
    default:
        return "Unknown";
    }
}

inline Layer StringToLayer(const char* string)
{
    LAYER_LIST(LAYER_IF)
    return Layer::DEFAULT;
}
