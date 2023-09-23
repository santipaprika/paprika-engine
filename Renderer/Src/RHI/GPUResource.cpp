#include <RHI/GPUResource.h>
#include <RHI/GPUResourceManager.h>

namespace PPK::RHI
{
	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource, DescriptorHeapHandle descriptorHeapHandle,
	                         D3D12_RESOURCE_STATES usageState)
		: m_resource(resource),
		  m_descriptorHeapHandle(descriptorHeapHandle),
		  m_usageState(usageState),
		  m_GPUAddress(0),
		  m_isReady(false)
	{
	}
	

	GPUResource::~GPUResource()
	{
		// TODO: Ensure proper destruction
		//m_resource->Release();
		m_resource = nullptr;
	}

}
