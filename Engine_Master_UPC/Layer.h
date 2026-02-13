#pragma once

#define LAYER_LIST \
    X(DEFAULT)     \
    X(UI)          \
    X(ENVIRONMENT) \
    X(PLAYER)      \
    X(ENEMY)       \
    X(PROJECTILE)

enum class Layer {
#define X(name) name,
    LAYER_LIST
#undef X
    COUNT
};

inline const char* LayerToString(Layer layer)
{
    switch (layer)
    {
#define X(name) case Layer::name: return #name;
        LAYER_LIST
#undef X
    default:
        return "Unknown";
    }
}
