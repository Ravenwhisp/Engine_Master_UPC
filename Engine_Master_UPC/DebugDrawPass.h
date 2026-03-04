#pragma once

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>

#include "IRenderPass.h"

class DDRenderInterfaceCoreD3D12;

// DebugDrawPass provides an interface for rendering debug geometry (lines, points, text, etc.) in a DirectX 12 application.
// It wraps the DebugDraw library's D3D12 implementation, manages its lifetime, and exposes a simple API for recording debug draw commands.
// Use this class to visualize geometry and diagnostics during development and debugging of graphics applications.
class DebugDrawPass: public IRenderPass
{

public:
    DebugDrawPass(ID3D12Device4* device, ID3D12CommandQueue* uploadQueue, bool useMSAA, D3D12_CPU_DESCRIPTOR_HANDLE cpuText = { 0 }, D3D12_GPU_DESCRIPTOR_HANDLE gpuText = { 0 });
    ~DebugDrawPass();


    void setView(const Matrix& view) { m_view = &view; }
    void setProjection(const Matrix& projection) { m_projection = &projection; }
    void setViewport(const D3D12_VIEWPORT& viewport) { m_viewport = &viewport; }

    void apply(ID3D12GraphicsCommandList4* commandList) override;
private:

    static DDRenderInterfaceCoreD3D12* implementation;

    mutable const D3D12_VIEWPORT* m_viewport = nullptr;

    mutable const Matrix* m_projection = nullptr;
    mutable const Matrix* m_view = nullptr;
};