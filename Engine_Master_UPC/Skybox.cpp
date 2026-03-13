#include "Globals.h"
#include "SkyBox.h"

#include "Application.h"
#include "ModuleResources.h"

#include "Texture.h"
#include "TextureAsset.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ModuleResources.h"
#include "ModuleFlyweight.h"
#include "TextureAsset.h"

struct SkyBoxVertex { Vector3 position; };

SkyBox::SkyBox(TextureAsset& asset)
{
    static const SkyBoxVertex vertexes[] =
    {
        {{-1, -1, -1}}, {{-1,  1, -1}}, {{ 1,  1, -1}}, {{ 1, -1, -1}},
        {{-1, -1,  1}}, {{-1,  1,  1}}, {{ 1,  1,  1}}, {{ 1, -1,  1}},
    };

    static const uint16_t indexes[] =
    {
        0,1,2, 0,2,3,
        4,6,5, 4,7,6,
        4,5,1, 4,1,0,
        3,2,6, 3,6,7,
        1,5,6, 1,6,2,
        4,0,3, 4,3,7
    };

    m_vertexBuffer.reset(app->getModuleResources()->createVertexBuffer(vertexes, _countof(vertexes), sizeof(SkyBoxVertex)));
    m_vertexBuffer->setName(L"Vertex SkyBox");
    m_indexBuffer.reset(app->getModuleResources()->createIndexBuffer(indexes, _countof(indexes), DXGI_FORMAT_R16_UINT, "IndexBuffer skybox"));
    m_texture = app->getModuleFlyweight()->createTexture(asset);
}

SkyBox::~SkyBox() = default;
