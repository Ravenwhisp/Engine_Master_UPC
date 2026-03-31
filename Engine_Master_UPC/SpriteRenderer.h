#pragma once

#include "Component.h"
#include "TextureAsset.h"

class Texture;

class SpriteRenderer : public Component
{
public:
    SpriteRenderer(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    bool consumeLoadRequest();

    Texture* getTexture() const { return m_texture; }

    TextureAsset* getTextureAsset() const { return m_textureAsset.get(); }
    MD5Hash getTextureAssetId() const { return m_textureAssetId; }

    bool hasTexture() const { return m_texture != nullptr; }

    void setGpuTexture(std::shared_ptr<Texture> texture);

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
    MD5Hash m_textureAssetId = INVALID_ASSET_ID;
    Texture* m_texture = nullptr;
    std::shared_ptr<Texture> m_gpuTexture = nullptr;
    std::shared_ptr<TextureAsset> m_textureAsset = nullptr;
    bool m_loadRequested = false;
};