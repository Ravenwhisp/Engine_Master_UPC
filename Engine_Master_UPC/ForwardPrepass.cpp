#include "Globals.h"
#include "ForwardPrepass.h"

ForwardPrepass::ForwardPrepass(ComPtr<ID3D12Device4> device)
{
	createRootSignature();
	createPipelineState();
}

void ForwardPrepass::createRootSignature()
{

}

void ForwardPrepass::createPipelineState()
{

}

void ForwardPrepass::prepare(const RenderContext& ctx)
{

}

void ForwardPrepass::apply(ID3D12GraphicsCommandList4* commandList)
{

}