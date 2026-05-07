#pragma once

#include "Component.h"
#include "TextureAsset.h"
#include "ParticleSystem.h"
#include "EmitterInstance.h"

class Texture;

class ParticleSystemComponent : public Component
{
public:

    ParticleSystemComponent(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void requestLoad() { m_loadRequested = true; }
    bool consumeLoadRequest();

    Texture* getTexture() const { return m_texture; }
    void setTexture(Texture* texture) { m_texture = texture; }

    void setTextureAssetId(const MD5Hash& assetId);

    TextureAsset* getTextureAsset() const { return m_textureAsset.get(); }
    MD5Hash getTextureAssetId() const { return m_textureAssetId; }

    bool hasTexture() const { return m_texture != nullptr; }

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:

    MD5Hash m_textureAssetId = INVALID_ASSET_ID;
    Texture* m_texture = nullptr;
    std::shared_ptr<Texture> m_gpuTexture = nullptr;
    std::shared_ptr<TextureAsset> m_textureAsset = nullptr;
    bool m_loadRequested = false;

    std::shared_ptr<ParticleSystem> m_particleSystem;
    std::vector<EmitterInstance> m_particlesState;
};

