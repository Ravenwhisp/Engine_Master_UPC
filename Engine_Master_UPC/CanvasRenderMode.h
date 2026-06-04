#pragma once
#include <cstring>

enum class CanvasRenderMode
{
    SCREEN_SPACE = 0,
    WORLD_SPACE,
    WORLD_SPACE_CAMERA
};

inline const char* CanvasRenderModeToString(uint32_t v)
{
    switch (static_cast<CanvasRenderMode>(v))
    {
    case CanvasRenderMode::SCREEN_SPACE:       return "SCREEN_SPACE";
    case CanvasRenderMode::WORLD_SPACE:        return "WORLD_SPACE";
    case CanvasRenderMode::WORLD_SPACE_CAMERA: return "WORLD_SPACE_CAMERA";
    default: return "SCREEN_SPACE";
    }
}

inline uint32_t StringToCanvasRenderMode(const char* s)
{
    if (std::strcmp(s, "SCREEN_SPACE") == 0)       return 0;
    if (std::strcmp(s, "WORLD_SPACE") == 0)        return 1;
    if (std::strcmp(s, "WORLD_SPACE_CAMERA") == 0) return 2;
    return 0;
}
