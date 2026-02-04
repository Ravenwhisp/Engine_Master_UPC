#pragma once

#include "Module.h"

#include <dxgi1_6.h>
#include <cstdint>
#include <chrono>
#include "DebugDrawPass.h"
#include "CommandQueue.h"
#include "DescriptorsModule.h"
#include "SwapChain.h"
#include "Model.h"
#include "Material.h"


// -----------------------------------------------------------------------------
// D3D12Module
// -----------------------------------------------------------------------------
class D3D12Module : public Module
{
public:
	D3D12Module(HWND hwnd);
	~D3D12Module();

	bool init() override;
	bool postInit() override;
	void preRender() override;
	void render() override;
	void postRender() override;
	bool cleanUp() override;

	ID3D12Device4* GetDevice() const { return m_device.Get(); }
	constexpr HWND GetWindowHandle() const { return _hwnd; }
	constexpr SwapChain* GetSwapChain() const { return _swapChain; }
	CommandQueue* GetCommandQueue() const { return _commandQueue.get(); }
	ID3D12GraphicsCommandList4* GetCommandList() const { return m_commandList.Get(); }

	IDXGIAdapter4* GetAdapter() const { return m_adapter.Get(); }

	uint64_t GetCurrentFrame() const { return m_fenceValues[m_frameIndex]; }
	uint64_t GetLastCompletedFrame() const { return m_lastCompletedFenceValue; }

	ComPtr<ID3D12RootSignature> CreateRootSignature();
	ComPtr<ID3D12PipelineState> CreatePipelineStateObject(ID3D12RootSignature* rootSignature);
private:

	void LoadPipeline();

	ComPtr<IDXGIAdapter4> m_adapter;
	// The DXGI factory used to create the swap chain and other DXGI objects
	ComPtr<IDXGIFactory6> m_dxgiFactory;
	// The main Direct3D 12 device interface used to create resources and command objects
	ComPtr<ID3D12Device4> m_device;

	ComPtr<ID3D12GraphicsCommandList4> m_commandList;

	//Synchronization values
	uint64_t m_frameIndex;
	uint64_t m_fenceValues[bufferCount];
	uint64_t m_lastCompletedFenceValue;

	std::unique_ptr<CommandQueue> _commandQueue;
	SwapChain* _swapChain;
	HWND _hwnd;

};