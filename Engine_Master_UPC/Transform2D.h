#pragma once
#include "Component.h"
#include "UIRect.h"

struct Float2
{
    float x = 0.0f;
    float y = 0.0f;
};

enum class StretchMode
{
    NONE = 0,
    HORIZONTAL = 1,
    VERTICAL = 2,
    BOTH = 3
};

enum class SizingMode
{
    KEEP_ASPECT_RATIO = 0,
    FIXED_SIZE = 1
};

class Transform2D : public Component
{
public:
    Transform2D(UID id, GameObject* owner);

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    Rect2D getRect() const;
    Rect2D getRect(const Rect2D& parent, const Vector2& inheritedScale = { 1.0f, 1.0f }) const;

    float getAlpha() const { return alpha; }
    void setAlpha(float a) { alpha = a; }
    float getInheritedAlpha(float parentAlpha = 1.0f) const { return alpha * parentAlpha; }

	Vector2 getPosition() const { return Vector2(position.x, position.y); }
	void setPosition(const Vector2& pos) { position.x = pos.x; position.y = pos.y; }
	Vector2 getScale() const { return Vector2(scale.x, scale.y); }
	void setScale(const Vector2& s) { scale.x = s.x; scale.y = s.y; }
    
    void setStretchMode(StretchMode mode);
    StretchMode getStretchMode() const { return stretchMode; }

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentValue) override;

public:
    Float2 position{ 0.0f, 0.0f };
    Float2 scale{ 1.0f, 1.0f };
    Float2 pivot{ 0.5f, 0.5f };

    float alpha = 1.0f;

    Float2 anchorMin{ 0.0f, 0.0f };
    Float2 anchorMax{ 0.0f, 0.0f };
    
    float aspectRatio = 1.0f;
    SizingMode sizingMode = SizingMode::KEEP_ASPECT_RATIO;
    
    Float2 baseSize{ 100.0f, 100.0f };

private:
    StretchMode stretchMode = StretchMode::NONE;
    
    void drawAnchorPresetsUI();
    void drawStretchPresetsUI();
    void setPointPreset(float ax, float ay, float px, float py);
    void setStretchPreset(StretchMode mode, float axMin, float ayMin, float axMax, float ayMax, float px, float py);
};