#pragma once
#include "IRenderPass.h"
#include "ResourceUploadBatch.h"
#include <SpriteBatch.h>
#include "SpriteFont.h"
#include "DescriptorHeap.h"
#include "UICommands.h"

class FontPass : public IRenderPass {
public:
	FontPass(ComPtr<ID3D12Device4> device);
	void apply(ID3D12GraphicsCommandList4* commandList) override;
	void setTextCommands(const std::vector<UITextCommand>* commands) { m_commands = commands; }
	void setViewport(const D3D12_VIEWPORT& viewport) { m_viewport = &viewport; }

private:
	void begin(ID3D12GraphicsCommandList4* commandList);
	void drawText(const wchar_t* text, float x, float y, const DirectX::XMFLOAT4& color, float scale);
	void end();

private:
	mutable const D3D12_VIEWPORT* m_viewport = nullptr;
	const std::vector<UITextCommand>* m_commands = nullptr;

	std::unique_ptr<DescriptorHeap>			m_fontHeap;

	std::unique_ptr<ResourceUploadBatch>	m_upload;
	std::unique_ptr<SpriteBatch>			m_spriteBatch;
	std::unique_ptr<SpriteFont>				m_spriteFont;

	ComPtr<ID3D12Device4>					m_device;
	ComPtr<ID3D12RootSignature>				m_rootSignature;
	ComPtr<ID3D12PipelineState>				m_pipelineState;
};