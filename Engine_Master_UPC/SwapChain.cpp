#include <Globals.h>
#include "SwapChain.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include "ModuleD3D12.h"
#include "CommandQueue.h"
#include "Texture.h"

SwapChain::SwapChain(HWND hWnd, ComPtr<ID3D12Device4> device, CommandQueue* queue): m_hwnd(hWnd), m_device(device), m_commandQueue(*queue)
{
    getWindowSize(m_windowWidth, m_windowHeight);
   
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    DXCall(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width =       m_windowWidth;
    swapChainDesc.Height =      m_windowHeight;
    swapChainDesc.Format =      DEFAULT_FORMAT;

    // UNORM = Unsigned normalized integer (0-255 mapped to 0.0-1.0)
    swapChainDesc.Stereo =      FALSE; // Set to TRUE for stereoscopic 3D rendering (VR/3D Vision)
    swapChainDesc.SampleDesc =  { 1, 0 }; // Multisampling { Count, Quality } // Count=1: No multisampling (1 sample per pixel)
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // This buffer will be used as a render target
    swapChainDesc.BufferCount = FRAMES_IN_FLIGHT;
    // - 1 front buffer (displayed)
   // - 1 back buffer (rendering)
    swapChainDesc.Scaling =     DXGI_SCALING_STRETCH; // How to scale when window size doesn't match buffer size:
    // STRETCH = Stretch the image to fit the window
    swapChainDesc.SwapEffect =  DXGI_SWAP_EFFECT_FLIP_DISCARD; // Modern efficient swap method:
    // - FLIP: Uses page flipping (no copying)
   // - DISCARD: Discard previous back buffer contents
    swapChainDesc.AlphaMode =   DXGI_ALPHA_MODE_UNSPECIFIED; // Alpha channel behavior for window blending UNSPECIFIED = Use default behavior
    swapChainDesc.Flags =       (checkTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
    // DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH: Allow full-screen mode switches
   //DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING: Allow tearing in windowed mode (VSync off)

    ComPtr<IDXGISwapChain1> swapChain1;
    DXCall(dxgiFactory4->CreateSwapChainForHwnd(
        m_commandQueue.getD3D12CommandQueue().Get(),
        hWnd,
        &swapChainDesc,
        nullptr, // fullscreen desc
        nullptr, // restrict output
        &swapChain1
    ));

    swapChain1.As(&m_swapChain);

    m_viewport = D3D12_VIEWPORT{ 0.0, 0.0, float(m_windowWidth), float(m_windowHeight) , 0.0, 1.0 };
    m_scissorRect = D3D12_RECT { 0, 0, long(m_windowWidth), long(m_windowHeight) };

    createRenderTargetViews(device);

    auto* depthTexture = app->getModuleResources()->createDepthBuffer(float(m_windowWidth), float(m_windowHeight));
    m_renderSurface.attachTexture( RenderSurface::DEPTH_STENCIL, std::shared_ptr<Texture>(depthTexture));
}

SwapChain::~SwapChain()
{
    // 3. Flush GPU commands
    app->getModuleD3D12()->getCommandQueue()->flush();

    // 4. Release swap chain
    m_swapChain.Reset();
    m_renderSurface.reset();
}

void SwapChain::present()
{
    m_swapChain.Get()->Present(0, checkTearingSupport() ? DXGI_PRESENT_ALLOW_TEARING : 0);
}

void SwapChain::getWindowSize(unsigned& width, unsigned& height) {
    RECT rect = {};
    GetClientRect(m_hwnd, &rect);

    width = unsigned(rect.right - rect.left);
    height = unsigned(rect.bottom - rect.top);
}

void SwapChain::resize()
{
    unsigned width, height;
    getWindowSize(width, height);

    if (width != m_windowWidth || height != m_windowHeight) {

        m_windowWidth = width;
        m_windowHeight = height;

        m_viewport = D3D12_VIEWPORT{ 0.0, 0.0, float(m_windowWidth), float(m_windowHeight) , 0.0, 1.0 };
        m_scissorRect = D3D12_RECT{ 0, 0, long(m_windowWidth), long(m_windowHeight) };

        app->getModuleD3D12()->getCommandQueue()->flush();

        // Release the render targets
        for (UINT n = 0; n < FRAMES_IN_FLIGHT; n++)
        {
            m_backBufferTextures[n]->release();
        }

        // Resize the swap chain
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

        DXCall(m_swapChain->GetDesc(&swapChainDesc));

        DXCall(m_swapChain->ResizeBuffers(FRAMES_IN_FLIGHT, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        createRenderTargetViews(app->getModuleD3D12()->getDevice());
        m_renderSurface.resize(m_windowWidth, m_windowHeight);
    }
}


void SwapChain::createRenderTargetViews(ComPtr<ID3D12Device2> device)
{    
    for (UINT n = 0; n < FRAMES_IN_FLIGHT; ++n)
    {
        ComPtr<ID3D12Resource> backBuffer;
        DXCall(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&backBuffer)));

#ifdef GAME_RELEASE
        constexpr DXGI_FORMAT rtvFmt = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
#else
        constexpr DXGI_FORMAT rtvFmt = DXGI_FORMAT_UNKNOWN;
#endif

        m_backBufferTextures[n] = app->getModuleResources()->createTexture(backBuffer, TextureView::RTV, rtvFmt);
        m_renderSurface.attachTexture(RenderSurface::COLOR_0, m_backBufferTextures[n]);
    }
}

bool SwapChain::checkTearingSupport() const
{
    BOOL allowTearing = FALSE;

    // Rather than create the DXGI 1.5 factory interface directly, we create the
    // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
    // graphics debugging tools which will not support the 1.5 factory interface 
    // until a future update.
    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            if (FAILED(factory5->CheckFeatureSupport( DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}

void SwapChain::updateCurrentBackBuffer()
{
    UINT index = m_swapChain->GetCurrentBackBufferIndex();
    m_renderSurface.attachTexture(RenderSurface::COLOR_0, m_backBufferTextures[index]);
}
const RenderSurface& SwapChain::getRenderSurface() const
{
    return m_renderSurface;
}
