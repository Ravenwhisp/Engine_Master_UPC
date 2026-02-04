#include "Globals.h"
#include "CommandQueue.h"

CommandQueue::CommandQueue(ComPtr<ID3D12Device4> device, D3D12_COMMAND_LIST_TYPE type)
    : m_FenceValue(0)
    , m_CommandListType(type)
    , m_d3d12Device(device)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    DXCall(m_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue)));

    DXCall(m_d3d12Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));
    m_d3d12Fence->SetName(L"CommandQueue Fence");
    m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(m_FenceEvent && "Failed to create fence event handle.");
}

CommandQueue::~CommandQueue()
{
    // 1. Wait for GPU to finish all work
    Flush();

    // 2. Close fence event handle
    if (m_FenceEvent)
    {
        CloseHandle(m_FenceEvent);
        m_FenceEvent = nullptr;
    }

    // 3. Reset command allocators
    while (!m_CommandAllocatorQueue.empty())
    {
        m_CommandAllocatorQueue.front().commandAllocator.Reset();
        m_CommandAllocatorQueue.pop();
    }

    // 4. Reset command lists
    while (!m_CommandListQueue.empty())
    {
        m_CommandListQueue.front().Reset();
        m_CommandListQueue.pop();
    }

    // 5. Reset fence
    m_d3d12Fence.Reset();

    // 6. Reset command queue
    m_d3d12CommandQueue.Reset();

    // 7. Reset device
    m_d3d12Device.Reset();
}

ComPtr<GraphicsCommandList> CommandQueue::GetCommandList()
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<GraphicsCommandList> commandList;

    if (!m_CommandAllocatorQueue.empty() && IsFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
    {
        commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
        m_CommandAllocatorQueue.pop();

        DXCall(commandAllocator->Reset());
    }
    else
    {
        commandAllocator = CreateCommandAllocator();
    }

    if (!m_CommandListQueue.empty())
    {
        commandList = m_CommandListQueue.front();
        m_CommandListQueue.pop();

        DXCall(commandList->Reset(commandAllocator.Get(), nullptr));
    }
    else
    {
        commandList = CreateCommandList(commandAllocator);
    }

    // Associate the command allocator with the command list so that it can be
    // retrieved when the command list is executed.
    DXCall(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

    return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(ComPtr<GraphicsCommandList> commandList)
{
    commandList->Close();

    ID3D12CommandAllocator* commandAllocator;
    UINT dataSize = sizeof(commandAllocator);
    DXCall(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

    ID3D12CommandList* const ppCommandLists[] = {
        commandList.Get()
    };

    m_d3d12CommandQueue->ExecuteCommandLists(std::size(ppCommandLists), ppCommandLists);
    uint64_t fenceValue = Signal();

    m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
    m_CommandListQueue.push(commandList);

    // The ownership of the command allocator has been transferred to the ComPtr
    // in the command allocator queue. It is safe to release the reference 
    // in this temporary COM pointer here.
    commandAllocator->Release();

    return fenceValue;
}

uint64_t CommandQueue::Signal()
{
    const uint64_t fenceToSignal = ++m_FenceValue;
    m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceToSignal);
    return fenceToSignal;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
    return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
    if (!IsFenceComplete(fenceValue))
    {
        m_d3d12Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }
}

uint64_t CommandQueue::GetCompletedFenceValue() const
{
    return m_d3d12Fence->GetCompletedValue();
}

void CommandQueue::Flush()
{
    // Signal a new fence value
    const UINT64 fenceToSignal = ++m_FenceValue;
    m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceToSignal);

    // Wait until GPU reaches it
    m_d3d12Fence->SetEventOnCompletion(fenceToSignal, m_FenceEvent);
    WaitForSingleObject(m_FenceEvent, INFINITE);
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetD3D12CommandQueue() const
{
    return m_d3d12CommandQueue;
}

ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    DXCall(m_d3d12Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)));
    commandAllocator->SetName(L"CommandAllocator" + m_CommandAllocatorQueue.size());
    return commandAllocator;
}

ComPtr<GraphicsCommandList> CommandQueue::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator)
{
    ComPtr<GraphicsCommandList> commandList;
    DXCall(m_d3d12Device->CreateCommandList(0, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    commandList->SetName(L"GraphicsCommandList" + +m_CommandListQueue.size());
    return commandList;
}
