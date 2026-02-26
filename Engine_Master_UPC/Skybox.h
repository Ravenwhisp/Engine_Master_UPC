#pragma once
#include <cstdint>
#include "Globals.h"
#include <TextureAsset.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>
#include <Texture.h>

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

    std::unique_ptr<VertexBuffer>&  getVertexBuffer() { return m_vertexBuffer; }
    std::unique_ptr<IndexBuffer>&   getIndexBuffer() { return m_indexBuffer; }
    std::unique_ptr<Texture>&       getTexture() { return m_texture; }
private:
    std::unique_ptr<VertexBuffer>   m_vertexBuffer;
    std::unique_ptr<IndexBuffer>    m_indexBuffer;

    std::unique_ptr<Texture>        m_texture;
};