#pragma once
#include "Module.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

#include "ModuleDescriptors.h"
#include "ImGuiPass.h"
#include "RenderViewType.h"
#include "SkinningComputePass.h"
#include "ShadowMapPass.h"
#include "PostProcessPass.h"
#include "BloomPass.h"

using Microsoft::WRL::ComPtr;

class ModuleGameView;
class Settings;
class RingBuffer;
class IRenderPass;
class RenderSurface;
class SkyBoxPass;
class MeshRendererPass;

struct ViewportEntry;
struct SkyBoxSettings;
class DebugDrawPass;

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
public:
    enum class ViewportType { EDITOR, PLAY };

    struct ViewportEntry
    {
        RenderSurface* surface = nullptr;
        ViewportType   type = ViewportType::EDITOR;
        float          width = 0.0f;
        float          height = 0.0f;
    };
private:
    Settings* m_settings = nullptr;
    ModuleGameView* m_moduleGameView = nullptr;

    RingBuffer* m_ringBuffer = nullptr;

    std::vector<std::unique_ptr<IRenderPass>> m_scenePasses;
    std::vector<std::unique_ptr<IRenderPass>> m_overlayPasses;
    std::unique_ptr<BloomPass> m_bloomPass;
    std::unique_ptr<PostProcessPass> m_postProcessPass;

    // ImGui straddles the frame (startFrame / apply), so it lives separately.
    std::unique_ptr<ImGuiPass> m_imGuiPass;

    // Cached viewports
    std::vector<ViewportEntry> m_viewports;

    bool m_pendingStopSimulation = false;

    DebugDrawPass* m_debugDrawPass = nullptr;
    MeshRendererPass* m_meshRenderPass = nullptr;

    SkyBoxPass* m_skyBoxPass;

    std::unique_ptr<SkinningComputePass> m_skinningComputePass;
    std::unique_ptr<ShadowMapPass> m_shadowMapPass;

    bool m_shadowMapRenderedThisFrame = false;
    const ShadowFrameData* m_currentShadowData = nullptr;

public:
    bool init()     override;
    void preRender() override;
    void render()   override;
    bool cleanUp()  override;

    void registerViewport(RenderSurface* surface, ViewportType type, float width, float height);

    SkyBoxPass* getSkyBoxPass() { return m_skyBoxPass; }

    D3D12_GPU_VIRTUAL_ADDRESS allocateInRingBuffer(const void* data, size_t size);

    int getTrianglesCount() const;
    int getMeshCount() const;
    void requestStopSimulation() { m_pendingStopSimulation = true; }

    // DebugDraw helper
    void markDebugDrawCacheDirty();

private:
    void renderToSurface( ID3D12GraphicsCommandList4* commandList, RenderSurface& surface, std::function<void(const RenderSurface& surface)> renderFunc);

    // Scene rendering
    void renderScene( ID3D12GraphicsCommandList4* commandList, const RenderCamera& camera, const RenderSurface& surface, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect, bool renderDebug, RenderViewType viewType);
    void renderBackground(ID3D12GraphicsCommandList4* commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, D3D12_VIEWPORT viewport, D3D12_RECT     scissorRect);
    void renderEditorScene(ID3D12GraphicsCommandList4* commandList, const RenderSurface& surface, float width, float height);
    void renderPlayScene(ID3D12GraphicsCommandList4* commandList, const RenderSurface& surface, float width, float height);
    void renderGameToBackbuffer( ID3D12GraphicsCommandList4* commandList, const RenderSurface& surface, D3D12_VIEWPORT viewport, D3D12_RECT     scissorRect);

    // Camera helpers
    RenderCamera getEditorCamera();
    RenderCamera getGameCamera();

    // D3D12 helpers
    void transitionResource( ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState,  D3D12_RESOURCE_STATES afterState);
};