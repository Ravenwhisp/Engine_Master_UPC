#include <Globals.h>
#include "SwapChain.h"
#include "Application.h"
#include "ResourcesModule.h"
#include "CameraModule.h"
#include "D3D12Module.h"

SwapChain::SwapChain(HWND hWnd): m_hwnd(hWnd)
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
    swapChainDesc.Format =      DEFAULT_FORMAT; // 32-bit RGBA format (8 bits per channel)
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
    auto commandQueue = app->getD3D12Module()->getCommandQueue()->getD3D12CommandQueue();
    DXCall(dxgiFactory4->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hWnd,
        &swapChainDesc,
        nullptr, // fullscreen desc
        nullptr, // restrict output
        &swapChain1
    ));

    swapChain1.As(&m_swapChain);

    m_depthStencil = app->getResourcesModule()->createDepthBuffer(m_windowWidth, m_windowHeight);
    createRenderTargetViews(app->getD3D12Module()->getDevice());

    m_viewport = D3D12_VIEWPORT{ 0.0, 0.0, float(m_windowWidth), float(m_windowHeight) , 0.0, 1.0 };
    m_scissorRect = D3D12_RECT { 0, 0, long(m_windowWidth), long(m_windowHeight) };
}

SwapChain::~SwapChain()
{
    m_depthStencil.reset();
    // 3. Flush GPU commands
    app->getD3D12Module()->getCommandQueue()->flush();

    // 4. Release swap chain
    m_swapChain.Reset();
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

        app->getD3D12Module()->getCommandQueue()->flush();

        // Release the render targets
        for (UINT n = 0; n < FRAMES_IN_FLIGHT; n++)
        {
            m_renderTargets[n].resource.Reset();
        }

        // Resize the swap chain
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

        DXCall(m_swapChain->GetDesc(&swapChainDesc));

        DXCall(m_swapChain->ResizeBuffers(FRAMES_IN_FLIGHT, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        // Recreate the render target views
        for (UINT n = 0; n < FRAMES_IN_FLIGHT; n++)
        {
            app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).free(m_renderTargets[n].rtv.handle);
            app->getResourcesModule()->defferResourceRelease(m_renderTargets[n].resource);
        }

        createRenderTargetViews(app->getD3D12Module()->getDevice());
        m_depthStencil = app->getResourcesModule()->createDepthBuffer(m_windowWidth, m_windowHeight);
        m_depthStencil->setName(L"SwapChainDS");
    }
}


void SwapChain::createRenderTargetViews(ComPtr<ID3D12Device2> device)
{    
    for (UINT n = 0; n < FRAMES_IN_FLIGHT; n++)
    {
        auto rtvHandle = app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV).allocate();
        m_renderTargets[n].rtv = rtvHandle;
        DXCall(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n].resource)));
        m_renderTargets[n].resource->SetName(L"BackBuffer");
        device->CreateRenderTargetView(m_renderTargets[n].resource.Get(), nullptr, rtvHandle.cpu);
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
            if (FAILED(factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}
