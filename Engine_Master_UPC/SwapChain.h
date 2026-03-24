#pragma once
#include "Globals.h"
#include "Resources.h"
#include <cstdint>
#include <dxgi1_6.h>
#include "Texture.h"
#include <RenderSurface.h>

using D3D12SwapChain = IDXGISwapChain4;


class CommandQueue;

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
	SwapChain(HWND hWnd, ComPtr<ID3D12Device4> device, CommandQueue* queue);
	~SwapChain();

	void present();
	void resize();
	void createRenderTargetViews(ComPtr<ID3D12Device2> device);
	void getWindowSize(unsigned& width, unsigned& height);
	bool checkTearingSupport() const;
	void updateCurrentBackBuffer();

	constexpr const D3D12_VIEWPORT& getViewport() { return m_viewport; }
	constexpr const D3D12_RECT&		getScissorRect() { return m_scissorRect; }
	constexpr uint32_t				getCurrentBackBufferIndex() const { return m_swapChain.Get()->GetCurrentBackBufferIndex(); }

	const RenderSurface&			getRenderSurface() const;
	const std::shared_ptr<Texture>& getCurrentRenderTarget() const { return m_backBufferTextures[getCurrentBackBufferIndex()]; }
private:

	ComPtr<ID3D12Device4>			m_device;
	CommandQueue&					m_commandQueue;
	ComPtr<D3D12SwapChain>			m_swapChain;
	std::shared_ptr<Texture> 		m_backBufferTextures[FRAMES_IN_FLIGHT]{};
	mutable RenderSurface			m_renderSurface;
	UINT index;

	UINT32							m_flags{ 0 };

	D3D12_VIEWPORT	m_viewport{};
	D3D12_RECT		m_scissorRect{};
	unsigned int	m_windowWidth{ 0 };
	unsigned int	m_windowHeight{ 0 };
	HWND			m_hwnd{ nullptr };
};

