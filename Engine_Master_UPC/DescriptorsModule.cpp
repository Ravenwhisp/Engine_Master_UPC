#include "Globals.h"
#include "DescriptorsModule.h"
#include "Application.h"
#include "D3D12Module.h"
#include "ResourcesModule.h"

DescriptorsModule::~DescriptorsModule()
{
    for (auto& pair : m_DescriptorHeapMap) {
        delete pair.second;
        pair.second = nullptr;
    }
    m_defferedDescriptors.clear();
}

bool DescriptorsModule::init()
{
    m_device = app->getD3D12Module()->getDevice();

    m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);
    m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256);
    m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096 * 8);
    m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = new DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SampleType::COUNT);

    createDefaultSamplers();
	return true;
}

void DescriptorsModule::preRender()
{
    // For now only the SRV heap is having deferred releases since it's the only one used for textures
	UINT lastCompletedFrame = app->getD3D12Module()->getLastCompletedFrame();
	for (int i = 0; i < m_defferedDescriptors.size(); ++i) {

        if (lastCompletedFrame > m_defferedDescriptors[i].frame)
        {
            m_DescriptorHeapMap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->free(m_defferedDescriptors[i].handle);
            m_defferedDescriptors[i] = m_defferedDescriptors.back();
            m_defferedDescriptors.pop_back();
        }
        else
        {
            ++i;
        }
	}
}

bool DescriptorsModule::cleanUp()
{

    return true;
}

void DescriptorsModule::createDefaultSamplers()
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

void DescriptorsModule::defferDescriptorRelease(Handle handle)
{
	DefferedDescriptor defferedDescriptor;
	defferedDescriptor.frame = app->getD3D12Module()->getCurrentFrame();
	defferedDescriptor.handle = handle;
	m_defferedDescriptors.push_back(defferedDescriptor);
}

