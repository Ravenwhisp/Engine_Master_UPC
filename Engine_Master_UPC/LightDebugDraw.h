#pragma once
class GameObject;

namespace LightDebugDraw
{
    void drawLightWithoutDepth(const GameObject& gameObject);
    void drawLightWithDepth(const GameObject& gameObject);
}