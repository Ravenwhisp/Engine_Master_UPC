#pragma once

#include <cstdint>
#include <memory>
#include "SimpleMath.h"

using DirectX::SimpleMath::Matrix;

class TextureAsset;
class VertexBuffer;
class IndexBuffer;
class Texture;

struct SkyParams
{
    Matrix vp;
    uint32_t flipX;
    uint32_t flipZ;
    uint32_t padding[2];
};

static_assert(sizeof(SkyParams) % 4 == 0);

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