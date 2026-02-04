#include "Globals.h"
#include "D3D12Module.h"
#include "Application.h"
#include "ResourcesModule.h"
#include "CameraModule.h"
#include "DescriptorsModule.h"
#include "EditorModule.h"
#include "Transform.h"
#include <d3dcompiler.h>
#include "RingBuffer.h"
#include <PlatformHelpers.h>

D3D12Module::D3D12Module(HWND hwnd) 
{
    _hwnd = hwnd;
}

D3D12Module::~D3D12Module()
{

}


bool D3D12Module::init()
{
    LoadPipeline();

    return true;
}

bool D3D12Module::postInit() {
    _swapChain = new SwapChain(_hwnd);
    return true;
}

void D3D12Module::preRender()
{
    m_frameIndex = _swapChain->GetCurrentBackBufferIndex();
    _commandQueue->WaitForFenceValue(m_fenceValues[m_frameIndex]);
    m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, m_fenceValues[m_frameIndex]);

    // Reset command list and allocator
    m_commandList = _commandQueue->GetCommandList();
}

void D3D12Module::render()
{

}

void D3D12Module::postRender()
{
    // Execute the command list.
    m_fenceValues[m_frameIndex] = _commandQueue->ExecuteCommandList(m_commandList);
    // Present the frame and allow tearing
    _swapChain->Present();
}

bool D3D12Module::cleanUp()
{
    if (_swapChain) {
        _swapChain->~SwapChain();
        delete _swapChain;
        _swapChain = nullptr;
    }

    // 5. Reset command list
    m_commandList.Reset();

    // 7. Reset command queue
    _commandQueue.reset();

    // 8. Reset device
    m_device.Reset();

    // 9. Reset DXGI factory
    m_dxgiFactory.Reset();

    return true;
}


void D3D12Module::LoadPipeline() {

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
    _commandQueue = std::make_unique<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
}


ComPtr<ID3D12RootSignature>  D3D12Module::CreateRootSignature() {

    ComPtr<ID3D12RootSignature> rootSignature;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[5] = {};
    CD3DX12_DESCRIPTOR_RANGE srvRange, sampRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // 1 range of 1 SRV descriptor
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, DescriptorsModule::SampleType::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL); // The descriptor table
    rootParameters[4].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(5, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

    return rootSignature;
}

ComPtr<ID3D12PipelineState> D3D12Module::CreatePipelineStateObject(ID3D12RootSignature* rootSignature) {
    ComPtr<ID3D12PipelineState> pso;
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    // Load the vertex shader.
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

    // Load the pixel shader.
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
     
    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc = { 1,0 };

    DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

    return pso;
}


