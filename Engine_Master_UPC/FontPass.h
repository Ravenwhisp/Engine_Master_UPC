#pragma once

#include "IRenderPass.h"
#include "DescriptorHeap.h"
#include "ResourceUploadBatch.h"

class Settings;
struct UITextCommand;

struct FontVertex
{
	DirectX::XMFLOAT2 position;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT4 color;
};

struct FontParams
{
	DirectX::XMFLOAT2 viewportSize;
	float time;
	float padding;
};

class FontPass : public IRenderPass
{
public:
	FontPass(ComPtr<ID3D12Device4> device);

	virtual void prepare(const RenderContext& ctx) override;

	void apply(ID3D12GraphicsCommandList4* commandList) override;
	void setTextCommands(const std::vector<UITextCommand>* commands) { m_commands = commands; }
	void setViewport(const D3D12_VIEWPORT& viewport) { m_viewport = &viewport; }

private:
	void begin(ID3D12GraphicsCommandList4* commandList);
	void drawText(ID3D12GraphicsCommandList4* commandList, int fontId, const wchar_t* text, float x, float y, const DirectX::XMFLOAT4& color, float scale);
	void end(ID3D12GraphicsCommandList4* commandList);

	void showDebugInformation(ID3D12GraphicsCommandList4* commandList);

private:
	Settings* m_settings = nullptr;

	const D3D12_VIEWPORT* m_viewport = nullptr;
	const std::vector<UITextCommand>* m_commands = nullptr;

	std::unique_ptr<DescriptorHeap> m_fontHeap;
	std::unique_ptr<ResourceUploadBatch> m_upload;

	std::vector<FontVertex> m_vertices;

	ComPtr<ID3D12Device4> m_device;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
};