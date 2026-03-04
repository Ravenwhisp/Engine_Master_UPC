#pragma once
#include "Component.h"
#include "UIRect.h"

struct Float2
{
    float x = 0.0f;
    float y = 0.0f;
};

class Transform2D : public Component
{
public:
    Transform2D(UID id, GameObject* owner);

    Rect2D getRect() const;
    Rect2D getRect(const Rect2D& parent) const;

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentValue) override;

public:
    Float2 position{ 0.0f, 0.0f };
    Float2 size{ 100.0f, 100.0f };
    Float2 pivot{ 0.5f, 0.5f };

    Float2 anchorMin{ 0.0f, 0.0f };
    Float2 anchorMax{ 0.0f, 0.0f };
private:
    void drawAnchorPresetsUI();
    void setPointPreset(float ax, float ay, float px, float py);

};