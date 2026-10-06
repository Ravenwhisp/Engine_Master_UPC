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
}

CommandQueue::~CommandQueue()
{
    // 1. Wait for GPU to finish all work
    flush();

    // 2. Reset command allocators
    while (!m_CommandAllocatorQueue.empty())
    {
        m_CommandAllocatorQueue.front().commandAllocator.Reset();
        m_CommandAllocatorQueue.pop();
    }

    // 3. Reset command lists
    while (!m_CommandListQueue.empty())
    {
        m_CommandListQueue.front().Reset();
        m_CommandListQueue.pop();
    }

    // 4. Reset fence
    m_d3d12Fence.Reset();

    // 5. Reset command queue
    m_d3d12CommandQueue.Reset();

    // 6. Reset device
    m_d3d12Device.Reset();
}

ComPtr<GraphicsCommandList> CommandQueue::getCommandList()
{
    // Only pool management is serialized. Once returned, each command list is
    // owned by its calling thread and can be recorded in parallel.
    std::unique_lock poolLock(m_PoolMutex);

    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<GraphicsCommandList> commandList;

    if (!m_CommandAllocatorQueue.empty() && isFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
    {
        commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
        m_CommandAllocatorQueue.pop();

        DXCall(commandAllocator->Reset());
    }
    else if (!m_CommandAllocatorQueue.empty() && m_CommandAllocatorQueue.size() >= MAX_POOL_SIZE)
    {
        waitForFenceValue(m_CommandAllocatorQueue.front().fenceValue);
        commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
        m_CommandAllocatorQueue.pop();

        DXCall(commandAllocator->Reset());
    }
    else
    {
        commandAllocator = createCommandAllocator();
    }

    if (!m_CommandListQueue.empty())
    {
        commandList = m_CommandListQueue.front();
        m_CommandListQueue.pop();

        DXCall(commandList->Reset(commandAllocator.Get(), nullptr));
    }
    else
    {
        commandList = createCommandList(commandAllocator);
    }

    // Associate the command allocator with the command list so that it can be
    // retrieved when the command list is executed.
    DXCall(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

    return commandList;
}

uint64_t CommandQueue::executeCommandList(ComPtr<GraphicsCommandList> commandList)
{
    DXCall(commandList->Close());

    ID3D12CommandAllocator* commandAllocator;
    UINT dataSize = sizeof(commandAllocator);
    DXCall(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

    ID3D12CommandList* const ppCommandLists[] = {
        commandList.Get()
    };

    uint64_t fenceValue = 0;
    {
        // A D3D12 queue is a single ordered stream. Serialize Execute + Signal
        // so fence values always describe the work immediately before them.
        std::lock_guard submissionLock(m_SubmissionMutex);
        m_d3d12CommandQueue->ExecuteCommandLists(std::size(ppCommandLists), ppCommandLists);
        fenceValue = ++m_FenceValue;
        DXCall(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceValue));
    }

    {
        std::lock_guard poolLock(m_PoolMutex);
        m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
        m_CommandListQueue.push(commandList);
    }

    // The ownership of the command allocator has been transferred to the ComPtr
    // in the command allocator queue. It is safe to release the reference 
    // in this temporary COM pointer here.
    commandAllocator->Release();

    return fenceValue;
}

uint64_t CommandQueue::signal()
{
    std::lock_guard submissionLock(m_SubmissionMutex);
    const uint64_t fenceToSignal = ++m_FenceValue;
    DXCall(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceToSignal));
    return fenceToSignal;
}

bool CommandQueue::isFenceComplete(uint64_t fenceValue)
{
    return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::waitForFenceValue(uint64_t fenceValue)
{
    if (!isFenceComplete(fenceValue))
    {
        // A per-wait event permits the render and loading threads to wait for
        // different fence values without racing on one auto-reset event.
        HANDLE fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        assert(fenceEvent && "Failed to create fence event handle.");
        if (!fenceEvent)
        {
            return;
        }

        DXCall(m_d3d12Fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    }
}

uint64_t CommandQueue::getCompletedFenceValue() const
{
    return m_d3d12Fence->GetCompletedValue();
}

void CommandQueue::flush()
{
    waitForFenceValue(signal());
}

ComPtr<ID3D12CommandQueue> CommandQueue::getD3D12CommandQueue() const
{
    return m_d3d12CommandQueue;
}

ComPtr<ID3D12CommandAllocator> CommandQueue::createCommandAllocator()
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    DXCall(m_d3d12Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator)));
    const std::wstring name = L"CommandAllocator " + std::to_wstring(m_CommandAllocatorQueue.size());
    commandAllocator->SetName(name.c_str());
    return commandAllocator;
}

ComPtr<GraphicsCommandList> CommandQueue::createCommandList(ComPtr<ID3D12CommandAllocator> allocator)
{
    ComPtr<GraphicsCommandList> commandList;
    DXCall(m_d3d12Device->CreateCommandList(0, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    const std::wstring name = L"GraphicsCommandList " + std::to_wstring(m_CommandListQueue.size());
    commandList->SetName(name.c_str());
    return commandList;
}
