#include "Globals.h"
#include "CubeLut.h"

#include "Texture.h"
#include "Application.h"
#include "ModuleResources.h"
#include "UID.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{
    // Creates the 3D texture and uploads the RGBA float data into it.
    std::shared_ptr<Texture> buildLutTexture(ID3D12Device4& device, int size, const std::vector<float>& rgba)
    {
        TextureDesc desc{};
        desc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        desc.width = static_cast<uint32_t>(size);
        desc.height = static_cast<uint32_t>(size);
        desc.depth = static_cast<uint16_t>(size);
        desc.mipLevels = 1;
        desc.views = TextureView::SRV;
        desc.initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        desc.shaderVisibleSRV = true;

        auto tex = std::make_shared<Texture>(GenerateUID(), device, desc);
        tex->setName(L"ColorGradingLUT");

        D3D12_SUBRESOURCE_DATA sub{};
        sub.pData = rgba.data();
        sub.RowPitch = static_cast<LONG_PTR>(size) * sizeof(float) * 4;
        sub.SlicePitch = sub.RowPitch * size;

        app->getModuleResources()->uploadTextureAndTransition(tex->getD3D12Resource().Get(), { sub });
        return tex;
    }
}

std::shared_ptr<Texture> CubeLut::createIdentity(ID3D12Device4& device, int size)
{
    if (size < 2) size = 2;

    std::vector<float> rgba(static_cast<size_t>(size) * size * size * 4);
    size_t idx = 0;
    const float inv = 1.0f / static_cast<float>(size - 1);

    for (int b = 0; b < size; ++b)
        for (int g = 0; g < size; ++g)
            for (int r = 0; r < size; ++r)
            {
                rgba[idx++] = r * inv;
                rgba[idx++] = g * inv;
                rgba[idx++] = b * inv;
                rgba[idx++] = 1.0f;
            }

    return buildLutTexture(device, size, rgba);
}

std::shared_ptr<Texture> CubeLut::load(ID3D12Device4& device, const std::vector<uint8_t>& data)
{
    if (data.empty())
    {
        DEBUG_ERROR("CubeLut: LUT asset contains no data.");
        return nullptr;
    }

    int size = 0;
    std::vector<float> rgba;
    size_t writeIdx = 0;
    const std::string text(data.begin(), data.end());
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.empty() || line[0] == '#' || line[0] == '\r')
            continue;

        int parsedSize = 0;
        if (size == 0 && sscanf_s(line.c_str(), "LUT_3D_SIZE %d", &parsedSize) == 1 && parsedSize > 0)
        {
            size = parsedSize;
            rgba.assign(static_cast<size_t>(size) * size * size * 4, 1.0f);
            continue;
        }

        if (size > 0)
        {
            float r = 0.0f, g = 0.0f, b = 0.0f;
            if (sscanf_s(line.c_str(), "%f %f %f", &r, &g, &b) == 3 && writeIdx + 4 <= rgba.size())
            {
                rgba[writeIdx++] = r;
                rgba[writeIdx++] = g;
                rgba[writeIdx++] = b;
                rgba[writeIdx++] = 1.0f;
            }
        }
    }

    if (size == 0 || writeIdx != rgba.size())
    {
        DEBUG_ERROR("CubeLut: malformed or incomplete LUT asset.");
        return nullptr;
    }

    return buildLutTexture(device, size, rgba);
}
