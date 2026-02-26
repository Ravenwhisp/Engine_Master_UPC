#include "Globals.h"
#include "Skybox.h"

#include "Application.h"
#include "ResourcesModule.h"

struct SkyboxVertex { Vector3 position; };


SkyBox::SkyBox(TextureAsset& asset)
{
    static const SkyboxVertex vertexes[] =
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

	m_vertexBuffer = app->getResourcesModule()->createVertexBuffer(vertexes, _countof(vertexes), sizeof(SkyboxVertex));
	m_indexBuffer = app->getResourcesModule()->createIndexBuffer(indexes, _countof(indexes), DXGI_FORMAT_R16_UINT);

    m_texture = app->getResourcesModule()->createTextureCubeFromFile(asset);
}
