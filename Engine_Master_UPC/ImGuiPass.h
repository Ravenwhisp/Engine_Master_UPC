#pragma once

class DescriptorHeap;

class ImGuiPass
{
    DescriptorHeap* heap;

public:
    ImGuiPass(ID3D12Device2* device, HWND hWnd, D3D12_CPU_DESCRIPTOR_HANDLE cpuTextHandle = { 0 }, D3D12_GPU_DESCRIPTOR_HANDLE gpuTextHandle = { 0 });
    ~ImGuiPass();

    DescriptorHeap* GetHeap() { return heap; }

    void startFrame();
    void record(ID3D12GraphicsCommandList* commandList);
};
