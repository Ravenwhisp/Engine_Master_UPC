#pragma once
#include "Module.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <array>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include "ModuleDescriptors.h"
#include "ImGuiPass.h"
#include "RenderViewType.h"
#include "SkinningComputePass.h"
#include "ShadowMapPass.h"
#include "SSAOTypes.h"
#include "SSAOGeometryPass.h"
#include "SSAOPass.h"
#include "SSAOBlurPass.h"
#include "DepthReductionPass.h"
#include "ShadowFrustumComputePass.h"
#include "LightCullingPass.h"
#include "VolumetricFogComputePass.h"
#include "OcclusionTargetDepthPass.h"
#include "DynamicTransparencyMaskPass.h"
#include "VideoPass.h"

using Microsoft::WRL::ComPtr;

class ModuleGameView;
class Settings;
class RingBuffer;
class IRenderPass;
class RenderSurface;
class SkyBoxPass;
class ForwardPrepass;
class DeferredShadingPass;
class GeometryPass;

struct ViewportEntry;
struct SkyBoxSettings;
class DebugDrawPass;

namespace DirectX { namespace SimpleMath { struct Matrix; struct Vector3; } }

using Matrix = DirectX::SimpleMath::Matrix;
using Vector3 = DirectX::SimpleMath::Vector3;

class ModuleRender : public Module
{
public:
    struct RenderPassTiming
    {
        std::string name;
        float cpuMs = 0.0f;
        float gpuMs = 0.0f;
    };

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
        float          pendingResizeWidth = 0.0f;
        float          pendingResizeHeight = 0.0f;
        bool           pendingResize = false;
        bool           isVisible = false;
    };
private:
    Settings* m_settings = nullptr;
    ModuleGameView* m_moduleGameView = nullptr;

    RingBuffer* m_ringBuffer = nullptr;
    // RingBuffer* m_structuredRingBuffer = nullptr; // mainly for particlesPass

    // Ordered list of passes that are called every frame.
    std::vector<std::unique_ptr<IRenderPass>> m_renderPasses;

    // ImGui straddles the frame (startFrame / apply), so it lives separately.
    std::unique_ptr<ImGuiPass> m_imGuiPass;

    // Cached viewports
    std::vector<ViewportEntry> m_viewports;

    bool m_pendingStopSimulation = false;

    DebugDrawPass* m_debugDrawPass = nullptr;
    ForwardPrepass* m_forwardPrepass = nullptr;
    GeometryPass* m_geometryPass = nullptr;
    DeferredShadingPass* m_meshRenderPass = nullptr;

    SkyBoxPass* m_skyBoxPass;

    std::unique_ptr<SkinningComputePass> m_skinningComputePass;
    std::unique_ptr<OcclusionTargetDepthPass> m_occlusionTargetDepthPass;
    std::unique_ptr<DynamicTransparencyMaskPass> m_dynamicTransparencyMaskPass;
    std::unique_ptr<DepthReductionPass> m_depthReductionPass;
    std::unique_ptr<ShadowFrustumComputePass> m_shadowFrustumComputePass;
    std::unique_ptr<LightCullingPass> m_lightCullingPass;
    std::unique_ptr<ShadowMapPass> m_shadowMapPass;
    std::unique_ptr<VolumetricFogComputePass> m_volumetricFogComputePass;
    std::unique_ptr<SSAOGeometryPass> m_ssaoGeometryPass;
    std::unique_ptr<SSAOPass> m_ssaoPass;
    std::unique_ptr<SSAOBlurPass> m_ssaoBlurPass;
    std::unique_ptr<VideoPass> m_videoPass;

    SSAOFrameData m_currentSSAOData{};

    static constexpr uint32_t MAX_PROFILED_RENDER_PASSES = 32;
    static constexpr uint32_t INVALID_PROFILE_INDEX = UINT32_MAX;

    ComPtr<ID3D12QueryHeap> m_timestampQueryHeap;
    ComPtr<ID3D12Resource> m_timestampReadbackBuffer;
    uint64_t* m_timestampReadbackData = nullptr;
    uint64_t m_timestampFrequency = 0;

    std::array<std::array<std::string, MAX_PROFILED_RENDER_PASSES>, FRAMES_IN_FLIGHT> m_gpuPassNames{};
    std::array<uint32_t, FRAMES_IN_FLIGHT> m_gpuPassCounts{};
    std::array<bool, FRAMES_IN_FLIGHT> m_gpuFramesPending{};
    std::unordered_map<std::string, float> m_latestGpuTimings;

    std::vector<RenderPassTiming> m_currentRenderTimings;
    std::vector<RenderPassTiming> m_displayRenderTimings;
    std::chrono::steady_clock::time_point m_currentCpuPassStart{};
    uint32_t m_profileFrameSlot = 0;
    uint32_t m_profilePassCount = 0;
    bool m_renderProfilingActive = false;

public:
    bool init()      override;
    void preRender() override;
    void render()    override;
    bool cleanUp()   override;

    void registerViewport(RenderSurface* surface, ViewportType type, float width, float height);
    void setViewportPendingResize(RenderSurface* surface, ViewportType type, float width, float height);
    void setViewportVisible(RenderSurface* surface, bool isVisible);
    void unregisterViewport(RenderSurface* surface);

    GeometryPass* getGeometryPass() { return m_geometryPass; }
    SkyBoxPass* getSkyBoxPass() { return m_skyBoxPass; }

    // (Re)creates the GBuffer/SSAO attachments for `surface` and rebuilds its
    // contiguous SRV descriptor table. Used for the swap-chain surface in
    // GAME_RELEASE (on init and on window resize).
    void createSceneRenderTargets(RenderSurface& surface, float width, float height);

    D3D12_GPU_VIRTUAL_ADDRESS allocateInRingBuffer(const void* data, size_t size);
    //D3D12_GPU_VIRTUAL_ADDRESS allocateInStructuredRingBuffer(const void* data, size_t size);

    int getTrianglesCount() const;
    int getMeshCount() const;
    const std::vector<RenderPassTiming>& getRenderPassTimings() const { return m_displayRenderTimings; }
    void requestStopSimulation() { m_pendingStopSimulation = true; }

    // DebugDraw helper
    void markDebugDrawCacheDirty();

    void resizeGameRenderTargets();

private:
    void initSceneRenderTargets(RenderSurface& surface, float width, float height);

    // Surface helpers
    void renderScene(ID3D12GraphicsCommandList4* commandList,const RenderCamera& camera,RenderSurface& outputSurface,bool renderDebug,RenderViewType viewType);

    void renderBackground(ID3D12GraphicsCommandList4* commandList,const RenderSurface& surface);

    // Wrappers called from preRender per registered viewport
    void renderEditorScene(ID3D12GraphicsCommandList4* commandList,RenderSurface& outputSurface);

    void renderPlayScene(ID3D12GraphicsCommandList4* commandList,RenderSurface& outputSurface);

    // GAME_RELEASE path � render directly to the swap-chain back-buffer
    void renderGameToBackbuffer(ID3D12GraphicsCommandList4* commandList,RenderSurface& outputSurface);

    // Camera helpers
    RenderCamera getEditorCamera();
    RenderCamera getGameCamera();

    // D3D12 helpers
    void transitionResource( ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState,  D3D12_RESOURCE_STATES afterState);

    bool renderVideo(ID3D12GraphicsCommandList4* commandList, RenderSurface& outputSurface);

    void initRenderProfiler(ID3D12Device4* device);
    void releaseRenderProfiler();
    void beginRenderProfiling(ID3D12GraphicsCommandList4* commandList, RenderViewType viewType);
    uint32_t beginRenderPassProfile(ID3D12GraphicsCommandList4* commandList, const char* name);
    void endRenderPassProfile(ID3D12GraphicsCommandList4* commandList, uint32_t profileIndex);
    void endRenderProfiling(ID3D12GraphicsCommandList4* commandList);
};
