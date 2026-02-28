#pragma once
#include "Component.h"
#include <string>
#include <TextureAsset.h>

class UIImage : public Component
{
public:
    UIImage(UID id, GameObject* owner);

    void requestLoad() { m_loadRequested = true; }              
    bool consumeLoadRequest();

    class Texture* getTexture() const { return m_texture; }
    void setTexture(Texture* texture) { m_texture = texture; }

    TextureAsset* getTextureAsset() const { return m_textureAsset; }
    UID getTextureAssetId() const { return m_textureAssetId; }

    bool hasTexture() const { return m_texture != nullptr; }

    void drawUi() override;

private:
    UID m_textureAssetId = 0;
    Texture* m_texture = nullptr;
    TextureAsset* m_textureAsset = nullptr;
    bool m_loadRequested = false;
};
