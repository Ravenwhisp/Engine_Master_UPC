#include "Globals.h"
#include "ModuleDescriptors.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"

ModuleDescriptors::ModuleDescriptors(ComPtr<ID3D12Device4> device)
{
    m_device = device;

    m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);
    m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256);
    m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096 * 8);
    m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = new DescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SampleType::COUNT);

    m_stagingSRVHeap = new StagingDescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096 * 8);

    createDefaultSamplers();
}

ModuleDescriptors::~ModuleDescriptors()
{
    for (auto& pair : m_DescriptorHeapMap) {
        delete pair.second;
        pair.second = nullptr;
    }
    delete m_stagingSRVHeap;
    m_defferedDescriptors.clear();
}

bool ModuleDescriptors::init()
{

	return true;
}

void ModuleDescriptors::preRender()
{
    UINT lastCompletedFrame = app->getModuleD3D12()->getLastCompletedFrame();
    for (int i = 0; i < (int)m_defferedDescriptors.size(); )
    {
        if (lastCompletedFrame > m_defferedDescriptors[i].frame)
        {
            m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->free(m_defferedDescriptors[i].handle);
            m_defferedDescriptors[i] = m_defferedDescriptors.back();
            m_defferedDescriptors.pop_back();
        }
        else ++i;
    }
}

bool ModuleDescriptors::cleanUp()
{

    return true;
}

void ModuleDescriptors::createDefaultSamplers()
{
    D3D12_SAMPLER_DESC samplers[COUNT] = {
        {
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
            {0.0f, 0.0f, 0.0f, 0.0f},
            0.0f, D3D12_FLOAT32_MAX
        },

        {
            D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0, 16, D3D12_COMPARISON_FUNC_NEVER,
            {0.0f, 0.0f, 0.0f, 0.0f},
            0.0f, D3D12_FLOAT32_MAX
        },

        {
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0, 16, D3D12_COMPARISON_FUNC_NEVER,
            {0.0f, 0.0f, 0.0f, 0.0f},
            0.0f, D3D12_FLOAT32_MAX
        },

        {
            D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0, 16, D3D12_COMPARISON_FUNC_NEVER,
            {0.0f, 0.0f, 0.0f, 0.0f},
            0.0f, D3D12_FLOAT32_MAX
        }
    };

    for (uint32_t i = 0; i < COUNT; ++i)
    {
        auto handle = m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->allocate();
        m_device->CreateSampler(&samplers[i], handle.cpu);
    }
}

void ModuleDescriptors::defferDescriptorRelease(Handle handle)
{
	DefferedDescriptor defferedDescriptor;
	defferedDescriptor.frame = app->getModuleD3D12()->getCurrentFrame();
	defferedDescriptor.handle = handle;
	m_defferedDescriptors.push_back(defferedDescriptor);
}

