#include <ApplicationHelper.h>
#include <Renderer.h>
#include <RHI/GPUResource.h>
#include <Logger.h>
#include <PassManager.h>

namespace PPK::RHI
{
	std::mutex g_ResourceCreationMutex;
	
	GPUResource::GPUResource(): m_GPUAddress(0), m_usageState(), m_isReady(false), m_sizeInBytes(0)
	{
	}

	static size_t GetResouceSize(ComPtr<ID3D12Resource> resource)
	{
		const D3D12_RESOURCE_DESC& resourceDesc = resource->GetDesc();
		D3D12_RESOURCE_ALLOCATION_INFO allocInfo = gDevice->GetResourceAllocationInfo(0, 1, &resourceDesc);
		return allocInfo.SizeInBytes;
	}

	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource, const DescriptorHeapHandles& descriptorHeapHandles,
							 D3D12_RESOURCE_STATES usageState, const std::string& name)
		: m_resource(resource),
		  m_descriptorHeapHandles(descriptorHeapHandles), // TODO: Add descriptors to memory report as well
		  m_usageState(usageState),
		  m_GPUAddress(resource->GetDesc().Height > 1 ? 0 : resource->GetGPUVirtualAddress()), //< Hacky way to detect if it's texture
		  m_isReady(false),
		  m_name(name)
	{
		Logger::Assert(resource, L"Attempting to initialize GPU resource proxy with NULL resource.");

		SetupResourceStats();
	}

	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState, const std::string& name)
		: m_resource(resource),
		  m_usageState(usageState),
		  m_GPUAddress(resource->GetDesc().Height > 1 ? 0 : resource->GetGPUVirtualAddress()), //< Hacky way to detect if it's texture
		  m_isReady(false),
		  m_name(name)
	{
		Logger::Assert(resource, L"Attempting to initialize GPU resource proxy with NULL resource.");

		SetupResourceStats();
	}

	GPUResource::GPUResource(GPUResource&& other) noexcept
	{
		m_resource = other.m_resource;
		other.m_resource = nullptr; // Don't delete since it will be used by copied moved object

		m_GPUAddress = other.m_GPUAddress;
		m_usageState = other.m_usageState;
		m_isReady = other.m_isReady;
		m_descriptorHeapHandles = other.m_descriptorHeapHandles;

		m_name = other.m_name;
		m_sizeInBytes = other.m_sizeInBytes;

		other.m_resource = nullptr;

		gResourcesMap[m_name] = this;

		Logger::Verbose(("Moving resource " + std::string(m_name)).c_str());
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
			m_descriptorHeapHandles = other.m_descriptorHeapHandles;

			m_name = other.m_name;
			m_sizeInBytes = other.m_sizeInBytes;
			gResourcesMap[m_name] = this;
		}


		return *this;
	}


	GPUResource::~GPUResource()
	{
		if (m_resource.Get())
		{
			// Assuming single thread accesses to gResourcesMap
			gResourcesMap.erase(m_name);
		}

		// TODO: We don't support dynamic deallocation of resource descriptors yet (CBV_SRV_UAV)
		for (int frameIdx = 0; frameIdx < gFrameCount; frameIdx++)
		{
			for (int heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV + 1; heapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; heapType++)
			{
				const DescriptorHeapHandle& handle = m_descriptorHeapHandles.handles[frameIdx][heapType];
				if (handle.IsValid())
				{
					gDescriptorHeapManager->FreeDescriptor(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(heapType), handle);
				}
			}
		}

		m_resource = nullptr;
	}

	void GPUResource::SetupResourceStats()
	{
		m_sizeInBytes = GetResouceSize(m_resource);
		gResourcesMap[m_name.c_str()] = this;
	}

	void GPUResource::AddDescriptorHandle(const DescriptorHeapHandle& heapHandle, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		uint32_t frameIdx)
	{
		Logger::Assert(heapHandle.IsValid(),
			L"Attempting to create GPU resource with null descriptor heap element. Please provide a valid one in constructor.");

		m_descriptorHeapHandles.handles[frameIdx][heapType] = heapHandle;
	}

	const DescriptorHeapHandle& GPUResource::GetDescriptorHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t frameIdx) const
	{
		Logger::Assert(m_descriptorHeapHandles.handles[frameIdx][static_cast<int>(heapType)].IsValid(),
			L"Attempting to get heap element that doesn't exist in current resource.");
		return m_descriptorHeapHandles.handles[frameIdx][heapType];
	}

	uint32_t GPUResource::GetIndexInRDH(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const
	{
		return m_descriptorHeapHandles.handles[0][heapType].GetHeapIndex(); //< TODO: Consider unified handle
	}


	size_t GPUResource::GetSizeInBytes() const
	{
		return m_sizeInBytes;
	}

	ComPtr<ID3D12Resource> GPUResourceUtils::CreateUninitializedGPUBuffer(size_t alignedSize, LPCSTR name, D3D12_RESOURCE_STATES outputState)
	{
		ComPtr<ID3D12Resource> bufferResource = nullptr;

		// Create a named variable for the heap properties
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

		// Create a named variable for the resource description
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);

		ThrowIfFailed(gDevice->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			outputState,
			nullptr,
		IID_PPV_ARGS(&bufferResource)));

		NAME_D3D12_OBJECT_CUSTOM(bufferResource, name);

		return bufferResource;
	}
}