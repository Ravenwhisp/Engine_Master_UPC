#pragma once
#include "Module.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

#include "ModuleDescriptors.h"
#include "ImGuiPass.h"

using Microsoft::WRL::ComPtr;

class ModuleGameView;
class Settings;
class RingBuffer;
class IRenderPass;
class RenderSurface;

struct SkyBoxSettings;

namespace DirectX { namespace SimpleMath { struct Matrix; struct Vector3; } }

using Matrix = DirectX::SimpleMath::Matrix;
using Vector3 = DirectX::SimpleMath::Vector3;

class ModuleRender : public Module
{
private:
    struct RenderCamera
    {
        Matrix   view;
        Matrix   projection;
        Vector3  position;
        bool     valid = false;
    };

private:
    Settings* m_settings = nullptr;
    ModuleGameView* m_moduleGameView = nullptr;

    RingBuffer* m_ringBuffer = nullptr;

    // One surface per viewport; each owns a colour RT + depth buffer.
    std::unique_ptr<RenderSurface> m_editorSurface;
    std::unique_ptr<RenderSurface> m_playSurface;

    // Ordered list of passes that are called every frame.
    std::vector<std::unique_ptr<IRenderPass>> m_renderPasses;

    // ImGui straddles the frame (startFrame / apply), so it lives separately.
    std::unique_ptr<ImGuiPass> m_imGuiPass;

    // Cached viewport size to detect resizes.
    ImVec2 m_size{ 800, 600 };

    int m_triangles = 0;

public:
    bool init()     override;
    void preRender() override;
    void render()   override;
    bool cleanUp()  override;

    D3D12_GPU_DESCRIPTOR_HANDLE getGPUEditorScreenRT();
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUPlayScreenRT();

    D3D12_GPU_VIRTUAL_ADDRESS allocateInRingBuffer(const void* data, size_t size);

    int getTriangles() const { return m_triangles; }

private:
    // Surface helpers
    std::unique_ptr<RenderSurface> createSurface(float width, float height);
    void                           resizeSurface(RenderSurface& surface, float width, float height);

    void renderToSurface( ID3D12GraphicsCommandList4* commandList, RenderSurface& surface, std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv)> renderFunc);

    // Scene rendering
    void renderScene( ID3D12GraphicsCommandList4* commandList, const RenderCamera& camera, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect, bool renderDebug);

    void renderBackground(  ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT     scissorRect);

    void renderEditorScene( ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, float width, float height);

    void renderPlayScene( ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,float width, float height);

    void renderGameToBackbuffer( ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT     scissorRect);

    // Camera helpers
    RenderCamera getEditorCamera();
    RenderCamera getGameCamera();

    // D3D12 helpers
    void transitionResource( ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState,  D3D12_RESOURCE_STATES afterState);
};