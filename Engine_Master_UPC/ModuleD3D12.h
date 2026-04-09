#pragma once

#include "Module.h"

#include <dxgi1_6.h>
#include <cstdint>
#include <chrono>
#include "DebugDrawPass.h"
#include "CommandQueue.h"
#include "ModuleDescriptors.h"
#include "SwapChain.h"
#include <GraphicsMemory.h>

class ModuleD3D12 : public Module
{
public:
	ModuleD3D12(HWND hwnd);
	~ModuleD3D12();

	bool init() override;
	void preRender() override;
	void postRender() override;
	bool cleanUp() override;

	ID3D12Device4*					getDevice() const { return m_device.Get(); }
	constexpr HWND					getWindowHandle() const { return m_hwnd; }
	constexpr SwapChain*			getSwapChain() const { return m_swapChain; }
	CommandQueue*					getCommandQueue() const { return m_commandQueue.get(); }
	ID3D12GraphicsCommandList4*		getCommandList() const { return m_commandList.Get(); }

	IDXGIAdapter4*					getAdapter() const { return m_adapter.Get(); }

	uint64_t						getCurrentFrame() const { return m_fenceValues[m_frameIndex]; }
	uint32_t						getCurrentFrameIndex() const { return static_cast<uint32_t>(m_frameIndex); }
	uint64_t						getLastCompletedFrame() const { return m_lastCompletedFenceValue; }

	ComPtr<ID3D12RootSignature>		createRootSignature();
	ComPtr<ID3D12PipelineState>		createPipelineStateObject(ID3D12RootSignature* rootSignature);

private:

	void loadPipeline();

	ComPtr<IDXGIAdapter4> m_adapter;
	ComPtr<IDXGIFactory6> m_dxgiFactory;
	ComPtr<ID3D12Device4> m_device;

	std::unique_ptr<GraphicsMemory> m_graphicsMemory;

	ComPtr<ID3D12GraphicsCommandList4> m_commandList;

	//Synchronization values
	uint64_t m_frameIndex;
	uint64_t m_fenceValues[FRAMES_IN_FLIGHT];
	uint64_t m_lastCompletedFenceValue;

	std::unique_ptr<CommandQueue>	m_commandQueue;
	SwapChain*						m_swapChain;
	HWND							m_hwnd;

};