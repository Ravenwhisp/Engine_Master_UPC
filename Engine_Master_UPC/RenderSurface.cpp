#include "Globals.h"
#include "RenderSurface.h"
#include "Texture.h"

RenderSurface::RenderSurface() : m_textures(AttachmentPoint::NUM_ATTACHMENT_POINTS)
, m_size(0, 0)
{
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

void RenderSurface::resize(Vector2 size)
{
	m_size = size;
	for (auto texture : m_textures) 
	{ 
		if (texture) {
			texture->resize(static_cast<uint32_t>(m_size.x), static_cast<uint32_t>(m_size.y));
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
