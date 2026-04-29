#pragma once

#include "Component.h"
#include "TextureAsset.h"
#include "AssetReference.h"

class Texture;

class SpriteRenderer : public Component
{
public:
    SpriteRenderer(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    bool consumeLoadRequest();

    Texture* getTexture() const { return m_texture; }

    TextureAsset* getTextureAsset() const { return m_textureAsset.get(); }
    AssetReference getTextureAssetId() const { return m_textureAssetId; }

    bool hasTexture() const { return m_texture != nullptr; }

    void setGpuTexture(std::shared_ptr<Texture> texture);

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

    bool getLookAtCamera() { return m_lookAtCamera; }

#pragma region Serialization
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::base_class<Component>(this), m_textureAssetId, m_lookAtCamera);
    }


    template<class Archive>
    static void load_and_construct(
        Archive& ar,
        cereal::construct<SpriteRenderer>& construct)
    {
        UID           id;
        ComponentType type;
        bool          active;
        ar(id, type, active);

        construct(id, nullptr);
        construct->setActive(active);

        ar(construct->m_textureAssetId, construct->m_lookAtCamera);
    }

#pragma endregion
private:
    AssetReference m_textureAssetId;
    Texture* m_texture = nullptr;
    std::shared_ptr<Texture> m_gpuTexture = nullptr;
    std::shared_ptr<TextureAsset> m_textureAsset = nullptr;
    bool m_loadRequested = false;
    bool m_lookAtCamera = false;


};

CEREAL_REGISTER_TYPE(SpriteRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, SpriteRenderer)