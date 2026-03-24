#pragma once

#include <memory>

class TextureAsset;
class VertexBuffer;
class IndexBuffer;
class Texture;

class SkyBox {
public:
    SkyBox(TextureAsset& asset);
	~SkyBox();

    std::unique_ptr<VertexBuffer>&  getVertexBuffer() { return m_vertexBuffer; }
    std::unique_ptr<IndexBuffer>&   getIndexBuffer() { return m_indexBuffer; }
    std::shared_ptr<Texture>&       getTexture() { return m_texture; }
private:
    std::unique_ptr<VertexBuffer>   m_vertexBuffer;
    std::unique_ptr<IndexBuffer>    m_indexBuffer;
    std::shared_ptr<Texture>        m_texture;
};