#pragma once
#include <d3d12.h>
#include <memory>
#include <string>

class Texture;

namespace CubeLut
{
    std::shared_ptr<Texture> load(ID3D12Device4& device, const std::string& path);

    std::shared_ptr<Texture> createIdentity(ID3D12Device4& device, int size);
}
