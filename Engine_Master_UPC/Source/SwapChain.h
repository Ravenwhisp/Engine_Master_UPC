#pragma once
#include <Globals.h>
#include "Resources.h"
#include <cstdint>
#include <dxgi1_6.h>

constexpr static uint32_t bufferCount = 3;

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
	constexpr static DXGI_FORMAT DefaultFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChain(HWND hWnd);
	~SwapChain();

	void Present();
	void Resize();
	void CreateRenderTargetViews(ComPtr<ID3D12Device2> device);
	void GetWindowSize(unsigned& width, unsigned& height);
	bool CheckTearingSupport() const;

	constexpr uint32_t Width() const { return (uint32_t)m_viewport.Width; }
	constexpr uint32_t Height() const { return (uint32_t)m_viewport.Height; }
	ID3D12Resource* GetCurrentRenderTarget() const { return m_renderTargets[GetCurrentBackBufferIndex()].resource.Get(); }
	DescriptorHandle GetCurrentRenderTargetView() const { return m_renderTargets[GetCurrentBackBufferIndex()].rtv; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() { return m_depthStencil->DSV().cpu; }
	constexpr const D3D12_VIEWPORT& GetViewport() { return m_viewport; }
	constexpr const D3D12_RECT& GetScissorRect() { return m_scissorRect; }
	constexpr uint32_t GetCurrentBackBufferIndex() const { return m_swapChain.Get()->GetCurrentBackBufferIndex(); }
	RenderTarget* GetRenderTargets() { return m_renderTargets; }
private:

	ComPtr<D3D12SwapChain> m_swapChain;
	RenderTarget m_renderTargets[bufferCount]{};
	std::unique_ptr<DepthBuffer> m_depthStencil;

	UINT32 _flags{ 0 };

	// Viewport and ScissorRect
	D3D12_VIEWPORT m_viewport{};
	D3D12_RECT m_scissorRect{};
	unsigned int windowWidth{ 0 };
	unsigned int windowHeight{ 0 };
	HWND _hwnd{ nullptr };
};

