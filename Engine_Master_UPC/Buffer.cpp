#include "Globals.h"
#include "Buffer.h"

Buffer::Buffer(ID3D12Device4& device, const D3D12_RESOURCE_DESC& resDesc): Resource(device, resDesc)
{

}

Buffer::Buffer(ID3D12Device4& device, ComPtr<ID3D12Resource> resource): Resource(device, resource)
{
}
