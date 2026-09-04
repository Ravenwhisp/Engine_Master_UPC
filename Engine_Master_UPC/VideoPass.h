#pragma once

#include "IRenderPass.h"
#include "DescriptorHeap.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <memory>
#include <vector>

extern "C"
{
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

using Microsoft::WRL::ComPtr;

class RenderSurface;
class VideoPlayback;

class VideoPass final : public IRenderPass
{
private:
	ComPtr<ID3D12Device4> m_device;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12Resource> m_videoTexture;

	ComPtr<ID3D12Resource> m_uploadBuffers[FRAMES_IN_FLIGHT];
	uint8_t* m_mappedUploadBuffers[FRAMES_IN_FLIGHT] = {};
	UINT64 m_uploadBufferSize = 0;

	std::unique_ptr<DescriptorHeap> m_descriptorHeap;
	std::vector<uint8_t> m_frameData;

	SwsContext* m_swsContext = nullptr;

	VideoPlayback* m_video = nullptr;
	RenderSurface* m_renderSurface = nullptr;

	D3D12_VIEWPORT m_viewport{};
	D3D12_RECT m_scissorRect{};

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	bool m_hasFrame = false;
	bool m_textureInShaderState = false;

public:
	explicit VideoPass(ComPtr<ID3D12Device4> device);
	~VideoPass() override;

	void prepare(const RenderContext& ctx) override;
	void prepare(RenderSurface& renderSurface, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect);

	void apply(ID3D12GraphicsCommandList4* commandList) override;
	bool render(ID3D12GraphicsCommandList4* commandList);

	void setVideo(VideoPlayback* video);

private:
	void createRootSignature();
	void createPipelineState();

	bool createVideoTexture(uint32_t width, uint32_t height);
	bool createUploadBuffers(UINT64 uploadSize);
	void releaseUploadBuffers();

	bool convertFrame(AVFrame* frame);
	bool updateVideoTexture(ID3D12GraphicsCommandList4* commandList, AVFrame* frame);

	void renderVideo(ID3D12GraphicsCommandList4* commandList);
};