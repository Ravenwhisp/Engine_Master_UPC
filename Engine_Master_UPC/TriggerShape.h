#pragma once
#include <cstring>

enum class TriggerShape
{
    Box = 0
};

inline const char* TriggerShapeToString(uint32_t v)
{
    (void)v;
    return "Box";
}

inline uint32_t StringToTriggerShape(const char* s)
{
    (void)s;
    return 0;
}
