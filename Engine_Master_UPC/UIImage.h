#pragma once
#include "Component.h"
#include <string>
#include <TextureAsset.h>

struct Rect2D;

enum class FillMethod
{
    Horizontal,
    Vertical,
    Radial90,
    Radial180,
    Radial360
};

class UIImage : public Component
{
public:
    UIImage(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void requestLoad() { m_loadRequested = true; }
    bool consumeLoadRequest();

    class Texture* getTexture() const { return m_texture; }
    void setTexture(Texture* texture) { m_texture = texture; }

    TextureAsset* getTextureAsset() const { return m_textureAsset.get(); }
    MD5Hash getTextureAssetId() const { return m_textureAssetId; }

    bool containsPoint(const Rect2D& rect, const Vector2& screenPos) const;

    bool hasTexture() const { return m_texture != nullptr; }

    float getFillAmount() const { return m_fillAmount; }
    void setFillAmount(float amount) { m_fillAmount = amount; }

    FillMethod getFillMethod() const { return m_fillMethod; }
    void setFillMethod(FillMethod method) { m_fillMethod = method; }

    bool getClockwise() const { return m_clockwise; }
    void setClockwise(bool clockwise) { m_clockwise = clockwise; }

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
    MD5Hash m_textureAssetId = INVALID_ASSET_ID;
    Texture* m_texture = nullptr;
    std::shared_ptr<TextureAsset> m_textureAsset = nullptr;
    bool m_loadRequested = false;

    float m_fillAmount = 1.0f;
    FillMethod m_fillMethod = FillMethod::Horizontal;
    bool m_clockwise = true;
};
