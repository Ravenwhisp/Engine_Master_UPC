#pragma once

struct RenderContext;

class IRenderPass 
{
public:
    virtual void prepare(const RenderContext& ctx) = 0;
    virtual void apply(ID3D12GraphicsCommandList4* commandList) = 0;
    virtual ~IRenderPass() = default;
};