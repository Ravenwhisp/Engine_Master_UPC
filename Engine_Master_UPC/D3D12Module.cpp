#include "Globals.h"
#include "D3D12Module.h"
#include <d3dcompiler.h>
#include <PlatformHelpers.h>

D3D12Module::D3D12Module(HWND hwnd) 
{
    m_hwnd = hwnd;
}

D3D12Module::~D3D12Module()
{

}


bool D3D12Module::init()
{
    loadPipeline();
    m_swapChain = new SwapChain(m_hwnd);
    m_graphicsMemory = std::make_unique<GraphicsMemory>(m_device.Get());

    return true;
}


void D3D12Module::preRender()
{
    m_frameIndex = m_swapChain->getCurrentBackBufferIndex();
    m_commandQueue->waitForFenceValue(m_fenceValues[m_frameIndex]);
    m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, m_fenceValues[m_frameIndex]);

    // Reset command list and allocator
    m_commandList = m_commandQueue->getCommandList();
}

void D3D12Module::render()
{

}

void D3D12Module::postRender()
{
    // Execute the command list.
    m_fenceValues[m_frameIndex] = m_commandQueue->executeCommandList(m_commandList);
    // Present the frame and allow tearing
    m_swapChain->present();

    m_graphicsMemory->Commit(m_commandQueue->getD3D12CommandQueue().Get());
}

bool D3D12Module::cleanUp()
{
    if (m_swapChain) {
        m_swapChain->~SwapChain();
        delete m_swapChain;
        m_swapChain = nullptr;
    }

    // 5. Reset command list
    m_commandList.Reset();

    // 7. Reset command queue
    m_commandQueue.reset();

    // 8. Reset device
    m_device.Reset();

    // 9. Reset DXGI factory
    m_dxgiFactory.Reset();

    return true;
}


void D3D12Module::loadPipeline() {

    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

#if defined(_DEBUG)
    CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_dxgiFactory));
#else
    CreateDXGIFactory2(0, IID_PPV_ARGS(&m_dxgiFactory));
#endif
    DXCall(m_dxgiFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter)));
    D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));

#ifdef  _DEBUG
    //Enable debugging layer. Requires Graphics Tools optional feature in Windows 10 SDK
    {
        ComPtr<ID3D12InfoQueue> info_queue;
        DXCall(m_device->QueryInterface(IID_PPV_ARGS(&info_queue)));

        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    }
#endif

    // Describe and create the command queue.
    m_commandQueue = std::make_unique<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
}


