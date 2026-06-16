#pragma once
#include "Component.h"
#include "UIRect.h"
#include <cstring>

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

inline const char* StretchModeToString(uint32_t v)
{
    switch (static_cast<StretchMode>(v))
    {
    case StretchMode::NONE:       return "NONE";
    case StretchMode::HORIZONTAL: return "HORIZONTAL";
    case StretchMode::VERTICAL:   return "VERTICAL";
    case StretchMode::BOTH:       return "BOTH";
    default: return "NONE";
    }
}

inline uint32_t StringToStretchMode(const char* s)
{
    if (std::strcmp(s, "NONE") == 0)       return 0;
    if (std::strcmp(s, "HORIZONTAL") == 0) return 1;
    if (std::strcmp(s, "VERTICAL") == 0)   return 2;
    if (std::strcmp(s, "BOTH") == 0)       return 3;
    return 0;
}

enum class SizingMode
{
    KEEP_ASPECT_RATIO = 0,
    FIXED_SIZE = 1
};

inline const char* SizingModeToString(uint32_t v)
{
    switch (static_cast<SizingMode>(v))
    {
    case SizingMode::KEEP_ASPECT_RATIO: return "KEEP_ASPECT_RATIO";
    case SizingMode::FIXED_SIZE:        return "FIXED_SIZE";
    default: return "KEEP_ASPECT_RATIO";
    }
}

inline uint32_t StringToSizingMode(const char* s)
{
    if (std::strcmp(s, "KEEP_ASPECT_RATIO") == 0) return 0;
    if (std::strcmp(s, "FIXED_SIZE") == 0)         return 1;
    return 0;
}

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
	Vector2 getBaseSize() const { return Vector2(baseSize.x, baseSize.y); }
	void setBaseSize(const Vector2& s) { baseSize.x = s.x; baseSize.y = s.y; }

	Vector2 getPivot() const { return Vector2(pivot.x, pivot.y); }
	void setPivot(const Vector2& p) { pivot.x = p.x; pivot.y = p.y; }
	Vector2 getAnchorMin() const { return Vector2(anchorMin.x, anchorMin.y); }
	void setAnchorMin(const Vector2& a) { anchorMin.x = a.x; anchorMin.y = a.y; }
	Vector2 getAnchorMax() const { return Vector2(anchorMax.x, anchorMax.y); }
	void setAnchorMax(const Vector2& a) { anchorMax.x = a.x; anchorMax.y = a.y; }
    
    void setStretchMode(StretchMode mode);
    StretchMode getStretchMode() const { return stretchMode; }

    void drawUi() override;

    void serialize(IArchive& archive) override;

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