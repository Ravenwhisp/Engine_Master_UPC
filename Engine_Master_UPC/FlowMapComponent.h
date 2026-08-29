#pragma once

#include "Component.h"
#include "TextureAsset.h"

class Texture;

enum class FlowMapSource : uint32_t
{
    DIRECTION = 0,
    TEXTURE = 1
};

enum class FlowMapTechnique : uint32_t
{
    DIRECTIONAL_SCROLL = 0,
    FLOW_MAP_WATER = 1
};

struct FlowMapData
{
    Vector2 direction = Vector2(1.0f, 0.0f);
    Vector2 tiling = Vector2::One;
    float speed = 0.1f;
    float strength = 0.1f;
    Vector2 offset = Vector2::Zero;
    uint32_t source = static_cast<uint32_t>(FlowMapSource::DIRECTION);
    uint32_t enabled = 1;
    uint32_t technique = static_cast<uint32_t>(FlowMapTechnique::DIRECTIONAL_SCROLL);
    float exaggeration = 1.0f;
    float textureStrength = 1.0f;
};

class FlowMapComponent : public Component
{
public:
    FlowMapComponent(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;
    void update() override;
    void drawUi() override;
    void serialize(IArchive& archive) override;
    void debugDraw() override {}
    void onTransformChange() override {}

    FlowMapData getData() const { return m_data; }
    FlowMapSource getSource() const { return static_cast<FlowMapSource>(m_data.source); }
    FlowMapTechnique getTechnique() const { return static_cast<FlowMapTechnique>(m_data.technique); }
    Texture* getTexture() const { return m_texture.get(); }
    float getPhase() const { return m_phase; }

    void setTextureAssetId(const AssetId& id);

private:
    void loadTexture();

    FlowMapData m_data{};
    AssetId m_textureAssetId{};
    std::shared_ptr<Texture> m_texture;
    std::shared_ptr<TextureAsset> m_textureAsset;
    float m_phase = 0.0f;
};
