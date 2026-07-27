#pragma once
#include <vector>
#include <memory>
#include <dxgiformat.h>

class Texture;
class DescriptorHeapBlock;

class RenderSurface 
{
public:
	enum AttachmentPoint: UINT {
		GBUFFER_DIFFUSE = 0,
		GBUFFER_SPECULAR = 1,
		GBUFFER_NORMAL = 2,
		GBUFFER_POSITION = 3,
		GBUFFER_EMISSIVE = 4,
		DEPTH_STENCIL = 5,
		COMPOSITE = 6,
		SSAO_DEPTH = 7,
		SSAO_NORMAL = 8,
		SSAO_RAW = 9,
		SSAO_BLUR = 10,
		SCENE_HDR = 11,   // HDR scene colour (lit scene before post-processing)
		NUM_ATTACHMENT_POINTS = 12
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

	void setSize(uint32_t width, uint32_t height);
	Vector2 getSize() const;

    uint32_t         getWidth() const;
    uint32_t         getHeight() const;


    const std::vector<std::shared_ptr<Texture>>& getTextures() const;

	void reset()
	{
		m_textures = RenderTargetList(AttachmentPoint::NUM_ATTACHMENT_POINTS);
	}

	void createDescriptorTable();
	void releaseDescriptorTable();
	D3D12_GPU_DESCRIPTOR_HANDLE getDescriptorTableGPUHandle() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getDescriptorTableCPUHandle(uint32_t slot) const;

private:
	using RenderTargetList = std::vector<std::shared_ptr<Texture>>;
	RenderTargetList m_textures;

	DescriptorHeapBlock* m_block{};

	Vector2 m_size;
};