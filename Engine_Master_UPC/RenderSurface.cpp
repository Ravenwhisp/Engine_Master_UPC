#include "Globals.h"
#include "RenderSurface.h"
#include "Texture.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "GeometryPass.h"
#include "ModuleResources.h"

RenderSurface::RenderSurface() : m_textures(AttachmentPoint::NUM_ATTACHMENT_POINTS)
, m_size(0, 0)
{
	createDescriptorTable();
}

void RenderSurface::attachTexture(AttachmentPoint attachmentPoint, std::shared_ptr<Texture> texture)
{
	m_textures[attachmentPoint] = texture;

	if (texture && texture->getD3D12Resource())
	{
		auto desc = texture->getD3D12ResourceDesc();

		m_size.x = static_cast<float>(desc.Width);
		m_size.y = static_cast<float>(desc.Height);
	}
}

std::shared_ptr<Texture> RenderSurface::getTexture(AttachmentPoint attachmentPoint) const
{
	return m_textures[attachmentPoint];
}

void RenderSurface::createDescriptorTable()
{
	DescriptorHeap& srvHeap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Allocate one contiguous block covering all GBuffer texture slots.
	m_block = srvHeap.allocateBlock(RenderSurface::NUM_ATTACHMENT_POINTS);
	assert(m_block && "Failed to allocate material descriptor block");
}

void RenderSurface::releaseDescriptorTable()
{
	for (int i = 0; i < m_block->size(); i++)
	{
		app->getModuleDescriptors()->defferDescriptorRelease((Handle)m_block->getHandle(i).handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

void RenderSurface::resize(Vector2 size)
{
	releaseDescriptorTable();
	createDescriptorTable();

	m_size = size;

	ID3D12Device* device = app->getModuleD3D12()->getDevice();
	DescriptorHeap& srvHeap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	int i = 0;
	for (auto texture : m_textures) 
	{ 
		if (texture) {
			texture->resize(static_cast<uint32_t>(m_size.x), static_cast<uint32_t>(m_size.y));

			if (!texture.get()->hasDSV())
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
				srvDesc.Format = texture.get()->getDesc().format;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Texture2D.MipLevels = 1;

				device->CreateShaderResourceView(
					texture.get()->getD3D12Resource().Get(),
					&srvDesc,
					getDescriptorTableCPUHandle(i)
				);
				i++;
			}
		}
	}
}

void RenderSurface::resize(uint32_t width, uint32_t height)
{
	resize(Vector2(static_cast<float>(width), static_cast<float>(height)));
}

Vector2 RenderSurface::getSize() const
{
	return m_size;
}

uint32_t RenderSurface::getWidth() const
{
	return static_cast<uint32_t>(m_size.x);
}

uint32_t RenderSurface::getHeight() const
{
	return static_cast<uint32_t>(m_size.y);
}

const std::vector<std::shared_ptr<Texture>>& RenderSurface::getTextures() const
{
	return m_textures;
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderSurface::getDescriptorTableGPUHandle() const
{
	assert(m_block && "RenderSurface descriptor table was not built");
	return m_block->getGPUHandle(0);
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderSurface::getDescriptorTableCPUHandle(uint32_t slot) const
{
	assert(m_block && "RenderSurface descriptor table was not built");
	return m_block->getCPUHandle(slot);

	/// 
}