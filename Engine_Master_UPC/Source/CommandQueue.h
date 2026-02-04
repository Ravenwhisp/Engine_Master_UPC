#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <queue>

using Microsoft::WRL::ComPtr;
using std::queue;

using GraphicsCommandList = ID3D12GraphicsCommandList4;

/// <summary>
/// This class provides a lightweight management layer around a DirectX 12
/// command queue. It maintains small pools of command allocators and command
/// lists, recycling them once the GPU has finished executing the work that
/// depends on them.
/// This class was implemented following this article: https://www.3dgep.com/learning-directx-12-2/
/// </summary>
class CommandQueue
{
public:
    CommandQueue(ComPtr<ID3D12Device4> device, D3D12_COMMAND_LIST_TYPE type);
    virtual ~CommandQueue();

    // Get an available command list from the command queue.
    ComPtr<GraphicsCommandList> GetCommandList();

    // Execute a command list.
    // Returns the fence value to wait for for this command list.
    uint64_t ExecuteCommandList(ComPtr<GraphicsCommandList> commandList);

    uint64_t Signal();
    bool IsFenceComplete(uint64_t fenceValue);
    void WaitForFenceValue(uint64_t fenceValue);
    uint64_t GetCompletedFenceValue() const;
    void Flush();

    ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;
protected:

    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
    ComPtr<GraphicsCommandList> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator);

private:
    // Keep track of command allocators that are "in-flight"
    struct CommandAllocatorEntry
    {
        uint64_t fenceValue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
    };

    using CommandAllocatorQueue = queue<CommandAllocatorEntry>;
    using CommandListQueue = queue<ComPtr<GraphicsCommandList>>;

    D3D12_COMMAND_LIST_TYPE                     m_CommandListType;
    ComPtr<ID3D12Device4>                       m_d3d12Device;
    ComPtr<ID3D12CommandQueue>                  m_d3d12CommandQueue;
    ComPtr<ID3D12Fence>                         m_d3d12Fence;
    HANDLE                                      m_FenceEvent;
    uint64_t                                    m_FenceValue;

    CommandAllocatorQueue                       m_CommandAllocatorQueue;
    CommandListQueue                            m_CommandListQueue;
};

