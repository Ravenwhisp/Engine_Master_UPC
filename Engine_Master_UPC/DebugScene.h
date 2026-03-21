#pragma once
#include "IDebugDrawer.h"

class GameObject;

class DebugScene : public IDebugDrawer
{
public:
    void draw(const RenderContext& ctx) override;
private:
    void drawGameObject(const GameObject* go) const;
};