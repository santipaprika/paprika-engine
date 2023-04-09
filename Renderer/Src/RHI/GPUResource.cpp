#include <RHI/GPUResource.h>

namespace PPK::RHI
{
	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState)
	{
		m_resource = resource;
		m_usageState = usageState;
		m_GPUAddress = 0;
		m_isReady = false;
	}

	GPUResource::~GPUResource()
	{
		// TODO: Ensure proper destruction
		//m_resource->Release();
		m_resource = nullptr;
	}

}