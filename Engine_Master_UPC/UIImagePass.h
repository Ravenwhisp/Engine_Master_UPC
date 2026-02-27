#pragma once

#include "IRenderPass.h"
#include "ResourceUploadBatch.h"
#include "DescriptorHeap.h"
#include <SpriteBatch.h>
#include "vector"
#include "UICommands.h"


class UIImagePass : public IRenderPass
{
public:
    explicit UIImagePass(ComPtr<ID3D12Device4> device);
    void apply(ID3D12GraphicsCommandList4* commandList) override;
    void setViewport(const D3D12_VIEWPORT& viewport) { m_viewport = &viewport; }
    void setImageCommands(const std::vector<UIImageCommand>* commands) { m_commands = commands; }

private:
    void begin(ID3D12GraphicsCommandList4* commandList);
    void drawImage(const UIImageCommand& command);
    void end();

private:
    mutable const D3D12_VIEWPORT* m_viewport = nullptr;
    const std::vector<UIImageCommand>* m_commands = nullptr;

    std::unique_ptr<DescriptorHeap>          m_heap;
    std::unique_ptr<ResourceUploadBatch>     m_upload;
    std::unique_ptr<SpriteBatch>             m_spriteBatch;

    ComPtr<ID3D12Device4>                    m_device;

    ComPtr<ID3D12RootSignature>              m_rootSignature;
    ComPtr<ID3D12PipelineState>              m_pipelineState;
};
