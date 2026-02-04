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

    ComPtr<GraphicsCommandList> getCommandList();
    ComPtr<ID3D12CommandQueue>  getD3D12CommandQueue() const;

    uint64_t                    executeCommandList(ComPtr<GraphicsCommandList> commandList);

    uint64_t    signal();
    bool        isFenceComplete(uint64_t fenceValue);
    void        waitForFenceValue(uint64_t fenceValue);
    uint64_t    getCompletedFenceValue() const;
    void        flush();

protected:

    ComPtr<ID3D12CommandAllocator>  createCommandAllocator();
    ComPtr<GraphicsCommandList>     createCommandList(ComPtr<ID3D12CommandAllocator> allocator);

private:
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

