#pragma once
#include "Component.h"
#include <string>
#include <TextureAsset.h>
#include "UIFill.h"

struct Rect2D;

class UIImage : public Component
{
public:
    UIImage(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void requestLoad() { m_loadRequested = true; }
    bool consumeLoadRequest();

    class Texture* getTexture() const { return m_texture; }
    void setTexture(Texture* texture) { m_texture = texture; }

    void setTextureAssetId(const AssetReference& assetId);

    TextureAsset* getTextureAsset() { return m_textureAsset.get(); }
    AssetReference& getTextureAssetId() { return m_textureAssetId; }

    bool containsPoint(const Rect2D& rect, const Vector2& screenPos) const;

    bool hasTexture() const { return m_texture != nullptr; }

    float getFillAmount() const { return m_fillAmount; }
    void setFillAmount(float amount) { m_fillAmount = amount; }

    FillMethod getFillMethod() const { return m_fillMethod; }
    void setFillMethod(FillMethod method) { m_fillMethod = method; }

    FillOrigin getFillOrigin() const { return m_fillOrigin; }
    void setFillOrigin(FillOrigin origin) { m_fillOrigin = origin; }

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
    AssetReference m_textureAssetId;
    Texture* m_texture = nullptr;
    std::shared_ptr<TextureAsset> m_textureAsset = nullptr;
    bool m_loadRequested = false;

    float m_fillAmount = 1.0f;
    FillMethod m_fillMethod = FillMethod::Horizontal;
    FillOrigin m_fillOrigin = FillOrigin::HorizontalLeft;

#pragma region Serialization
	template<class Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<Component>(this), m_textureAssetId, m_fillAmount, m_fillMethod, m_fillOrigin);
	}
#pragma endregion
};

CEREAL_REGISTER_TYPE(UIImage);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, UIImage)
