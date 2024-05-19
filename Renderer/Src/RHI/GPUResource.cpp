#include <Renderer.h>
#include <RHI/GPUResource.h>
#include <RHI/DescriptorHeapManager.h>

namespace PPK::RHI
{
	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource, std::shared_ptr<DescriptorHeapElement> descriptorHeapElement,
	                         D3D12_RESOURCE_STATES usageState)
		: m_resource(resource),
		  m_descriptorHeapElement(descriptorHeapElement),
		  m_usageState(usageState),
		  m_GPUAddress(0),
		  m_isReady(false)
	{
	}
	

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
