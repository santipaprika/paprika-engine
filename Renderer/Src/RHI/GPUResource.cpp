#include <Renderer.h>
#include <RHI/GPUResource.h>
#include <Logger.h>

namespace PPK::RHI
{
	GPUResource::GPUResource(): m_GPUAddress(0), m_usageState(), m_isReady(false)
	{
	}

	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource, std::shared_ptr<DescriptorHeapElement> descriptorHeapElement,
	                         D3D12_RESOURCE_STATES usageState, const std::wstring& name)
		: m_resource(resource),
		  m_descriptorHeapElement(descriptorHeapElement),
		  m_usageState(usageState),
		  m_GPUAddress(0),
		  m_isReady(false),
		  m_name(name)
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

		m_name = other.m_name;

		Logger::Info((L"Moving resource " + std::wstring(m_name)).c_str());
	}

	GPUResource& GPUResource::operator=(GPUResource&& other) noexcept
	{
		if (this != &other)
		{
			m_resource.Reset();
			m_resource = other.m_resource;
			other.m_resource = nullptr; // Don't delete since it will be used by copied moved object
	
			m_GPUAddress = other.m_GPUAddress;
			m_usageState = other.m_usageState;
			m_isReady = other.m_isReady;
			m_descriptorHeapElement = other.m_descriptorHeapElement;
			m_name = other.m_name;
		}
		return *this;
	}


	GPUResource::~GPUResource()
	{
		if (m_resource.Get())
		{
			m_resource->Unmap(0, NULL);
		}

		m_resource = nullptr;
	}

	void GPUResource::CopyDescriptorsToShaderHeap(D3D12_CPU_DESCRIPTOR_HANDLE currentCBVHandle, uint32_t descriptorIndex) const
	{
		const uint32_t cbvDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		
		D3D12_CPU_DESCRIPTOR_HANDLE indexedHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(currentCBVHandle, descriptorIndex, cbvDescriptorSize); 
		gDevice->CopyDescriptorsSimple(1, indexedHandle, GetDescriptorHeapElement()->GetCPUHandle(), GetDescriptorHeapElement()->GetHeapType());
	}
}
