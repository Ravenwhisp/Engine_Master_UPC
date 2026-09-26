#pragma once
#include <d3d12.h>
#include <memory>
#include <string>
#include <vector>

class Texture;

namespace CubeLut
{
    std::shared_ptr<Texture> load(ID3D12Device4& device, const std::vector<uint8_t>& data);

    std::shared_ptr<Texture> createIdentity(ID3D12Device4& device, int size);
}
