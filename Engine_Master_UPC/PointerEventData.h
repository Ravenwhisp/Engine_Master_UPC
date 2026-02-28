#pragma once
#include "Globals.h"

enum class PointerButton : uint8_t
{
    Left = 0,
    Right = 1,
    Middle = 2
};

class GameObject;

struct PointerEventData
{
    GameObject* pointerEnter = nullptr;
    GameObject* pointerPress = nullptr;
    GameObject* pointerClick = nullptr;
    Vector2        position = { 0,0 };
    Vector2        delta = { 0,0 };
    Vector2        pressPosition = { 0,0 };
    PointerButton button = PointerButton::Left;
};