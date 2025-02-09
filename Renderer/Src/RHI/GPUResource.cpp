#include <Renderer.h>
#include <RHI/GPUResource.h>

namespace PPK::RHI
{
	GPUResource::GPUResource(): m_GPUAddress(0), m_usageState(), m_isReady(false)
	{
	}

	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource, std::shared_ptr<DescriptorHeapElement> descriptorHeapElement,
	                         D3D12_RESOURCE_STATES usageState)
		: m_resource(resource),
		  m_descriptorHeapElement(descriptorHeapElement),
		  m_usageState(usageState),
		  m_GPUAddress(0),
		  m_isReady(false)
	{
	}

	GPUResource::GPUResource(GPUResource&& other) noexcept
	{
		m_resource = other.m_resource;
		other.m_resource = nullptr; // Don't delete since it will be used by copied moved object

		m_GPUAddress = other.m_GPUAddress;
		m_usageState = other.m_usageState;
		m_isReady = other.m_isReady;
		m_descriptorHeapElement = other.m_descriptorHeapElement;
	}

	// GPUResource& GPUResource::operator=(GPUResource&& other) noexcept
	// {
	// 	if (this != &other)
	// 	{
	// 		delete m_resource.Get();
	// 		m_resource = other.m_resource;
	// 		other.m_resource = nullptr; // Don't delete since it will be used by copied moved object
	//
	// 		m_GPUAddress = other.m_GPUAddress;
	// 		m_usageState = other.m_usageState;
	// 		m_isReady = other.m_isReady;
	// 		m_descriptorHeapElement = other.m_descriptorHeapElement;
	// 	}
	// 	return *this;
	// }


	GPUResource::~GPUResource()
	{
		if (m_resource.Get())
		{
			m_resource->Unmap(0, NULL);
		}

		m_resource = nullptr;
	}

	void GPUResource::CopyDescriptorsToShaderHeap(D3D12_CPU_DESCRIPTOR_HANDLE& currentCBVHandle)
	{
		const uint32_t cbvDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		gDevice->CopyDescriptorsSimple(1, currentCBVHandle, GetDescriptorHeapElement()->GetCPUHandle(), GetDescriptorHeapElement()->GetHeapType());
		currentCBVHandle.ptr += cbvDescriptorSize;
	}
}
