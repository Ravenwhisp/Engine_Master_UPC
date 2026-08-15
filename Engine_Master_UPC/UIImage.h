#pragma once
#include "Component.h"
#include <string>
#include <TextureAsset.h>
#include "UIFill.h"

class Texture;
struct Rect2D;

class UIImage : public Component
{
public:
    enum class StretchDrawMode
    {
        Stretch = 0,
        Tile = 1,
        Cover = 2
    };

    static const char* StretchDrawModeToString(uint32_t v)
    {
        switch (static_cast<StretchDrawMode>(v))
        {
        case StretchDrawMode::Stretch: return "Stretch";
        case StretchDrawMode::Tile:    return "Tile";
        case StretchDrawMode::Cover:   return "Cover";
        }
    }

    static uint32_t StringToStretchDrawMode(const char* s)
    {
        if (std::strcmp(s, "Tile") == 0)  return 1;
        if (std::strcmp(s, "Cover") == 0) return 2;
        return 0;
    }

    UIImage(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void requestLoad() { m_loadRequested = true; }
    bool consumeLoadRequest();

    Texture* getTexture() const { return m_texture; }
    void setTexture(Texture* texture) { m_texture = texture; }

    void setTextureAssetId(const AssetId& assetId);

    TextureAsset* getTextureAsset() const { return m_textureAsset.get(); }
    const AssetId& getTextureAssetId() const { return m_textureAssetId; }

    bool containsPoint(const Rect2D& rect, const Vector2& screenPos) const;

    bool hasTexture() const { return m_texture != nullptr; }

    Vector2 getFillAmount() const { return m_fillAmount; }
    void setFillAmount(const Vector2& amount) { m_fillAmount = amount; }

    FillMethod getFillMethod() const { return m_fillMethod; }
    void setFillMethod(FillMethod method) { m_fillMethod = method; }

    FillOrigin getFillOrigin() const { return m_fillOrigin; }
    void setFillOrigin(FillOrigin origin) { m_fillOrigin = origin; }

    int getSheetColumns() const { return m_sheetColumns; }
    int getSheetRows() const { return m_sheetRows; }
    Vector2 getSheetOffset() const { return m_sheetOffset; }

    void setSheetGrid(int columns, int rows)
    {
        m_sheetColumns = std::max(1, columns);
        m_sheetRows = std::max(1, rows);
    }

    void setSheetOffset(const Vector2& offset) { m_sheetOffset = offset; }

    StretchDrawMode getStretchDrawMode() const { return m_stretchDrawMode; }
    void setStretchDrawMode(StretchDrawMode mode) { m_stretchDrawMode = mode; }

    void drawUi() override;

    void serialize(IArchive& archive) override;
    void fixReferences(const SceneReferenceResolver& resolver) override;

private:
    AssetId m_textureAssetId{};
    Texture* m_texture = nullptr;
    std::shared_ptr<TextureAsset> m_textureAsset = nullptr;
    bool m_loadRequested = false;

    Vector2 m_fillAmount = { 0.0f, 1.0f };
    FillMethod m_fillMethod = FillMethod::Horizontal;
    FillOrigin m_fillOrigin = FillOrigin::HorizontalLeft;

    int m_sheetColumns = 1;
    int m_sheetRows = 1;
    Vector2 m_sheetOffset = { 0.0f, 0.0f };

    StretchDrawMode m_stretchDrawMode = StretchDrawMode::Stretch;
};
