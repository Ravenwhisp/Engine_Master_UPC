#pragma once
#include <vector>
#include <memory>
#include <dxgiformat.h>

class Texture;

class RenderSurface 
{
public:
	enum AttachmentPoint {
		COLOR_0,
		COLOR_1,
		DEPTH_STENCIL,
		NUM_ATTACHMENT_POINTS
	};


	RenderSurface();

	RenderSurface(const RenderSurface& copy) = default;
	RenderSurface(RenderSurface&& copy) = default;

	RenderSurface& operator=(const RenderSurface& other) = default;
	RenderSurface& operator=(RenderSurface&& other) = default;

    void                     attachTexture(AttachmentPoint attachmentPoint, std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> getTexture(AttachmentPoint attachmentPoint) const;

	void             resize(Vector2 size);
    void             resize(uint32_t width, uint32_t height);

	Vector2 getSize() const;

    uint32_t         getWidth() const;
    uint32_t         getHeight() const;


    const std::vector<std::shared_ptr<Texture>>& getTextures() const;

	void reset()
	{
		m_textures = RenderTargetList(AttachmentPoint::NUM_ATTACHMENT_POINTS);
	}

private:
	using RenderTargetList = std::vector<std::shared_ptr<Texture>>;
	RenderTargetList m_textures;

	Vector2 m_size;
};