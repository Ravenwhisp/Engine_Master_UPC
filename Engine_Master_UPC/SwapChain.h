#pragma once
#include <Globals.h>
#include "Resources.h"
#include <cstdint>
#include <dxgi1_6.h>
#include <DepthBuffer.h>

using D3D12SwapChain = IDXGISwapChain4;

//
// -------------------------------------------------------------------------------------------------
// Window
// -------------------------------------------------------------------------------------------------
// Encapsulates a Win32 window and its associated DirectX 12 rendering resources:
// - Swap chain (triple-buffered)
// - Render targets for each back buffer
// - Depth buffer
// - Viewport and scissor rect
//
class SwapChain
{
public:
	constexpr static DXGI_FORMAT DEFAULT_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChain(HWND hWnd);
	~SwapChain();

	void present();
	void resize();
	void createRenderTargetViews(ComPtr<ID3D12Device2> device);
	void getWindowSize(unsigned& width, unsigned& height);
	bool checkTearingSupport() const;

	constexpr uint32_t		width() const { return (uint32_t)m_viewport.Width; }
	constexpr uint32_t		height() const { return (uint32_t)m_viewport.Height; }

	ID3D12Resource*					getCurrentRenderTarget() const { return m_renderTargets[getCurrentBackBufferIndex()].resource.Get(); }
	DescriptorHandle				getCurrentRenderTargetView() const { return m_renderTargets[getCurrentBackBufferIndex()].rtv; }
	D3D12_CPU_DESCRIPTOR_HANDLE		getDepthStencilView() { return m_depthStencil->getDSV().cpu; }
	constexpr const D3D12_VIEWPORT& getViewport() { return m_viewport; }
	constexpr const D3D12_RECT&		getScissorRect() { return m_scissorRect; }
	constexpr uint32_t				getCurrentBackBufferIndex() const { return m_swapChain.Get()->GetCurrentBackBufferIndex(); }
	RenderTarget*					getRenderTargets() { return m_renderTargets; }
private:

	ComPtr<D3D12SwapChain>			m_swapChain;
	RenderTarget					m_renderTargets[FRAMES_IN_FLIGHT]{};
	std::unique_ptr<DepthBuffer>	m_depthStencil;

	UINT32							m_flags{ 0 };

	D3D12_VIEWPORT	m_viewport{};
	D3D12_RECT		m_scissorRect{};
	unsigned int	m_windowWidth{ 0 };
	unsigned int	m_windowHeight{ 0 };
	HWND			m_hwnd{ nullptr };
};

