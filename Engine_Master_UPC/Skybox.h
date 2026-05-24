#pragma once

#include <memory>

class TextureAsset;
class VertexBuffer;
class IndexBuffer;
class Texture;

class SkyBox {
public:

    struct EnvironmentData 
    {
        float roughness;
        Vector3 padding;
    };

    SkyBox(TextureAsset& asset);
	~SkyBox();

    std::unique_ptr<VertexBuffer>&  getVertexBuffer() { return m_vertexBuffer; }
    std::unique_ptr<IndexBuffer>&   getIndexBuffer() { return m_indexBuffer; }
    std::shared_ptr<Texture>&       getHdrTexture() { return m_hdrTexture; }
    std::shared_ptr<Texture>&       getTexture() { return m_texture; }
    std::shared_ptr<Texture>&       getIrradiance() { return m_irradiance; }
    std::shared_ptr<Texture>&       getEnvironment() { return m_environment; }
private:
    std::unique_ptr<VertexBuffer>   m_vertexBuffer;
    std::unique_ptr<IndexBuffer>    m_indexBuffer;
    std::shared_ptr<Texture>        m_hdrTexture;
    std::shared_ptr<Texture>        m_texture;
    std::shared_ptr<Texture>        m_irradiance;
    std::shared_ptr<Texture>        m_environment;
};