#include "Globals.h"
#include "SSAOGeometryPass.h"

#include "RenderContext.h"
#include "OptickProfiler.h"

SSAOGeometryPass::SSAOGeometryPass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
}

void SSAOGeometryPass::prepare(const RenderContext& ctx)
{
    PERF_RENDER("SSAOGeometryPass::prepare");
}

void SSAOGeometryPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    PERF_RENDER("SSAOGeometryPass::apply");
}