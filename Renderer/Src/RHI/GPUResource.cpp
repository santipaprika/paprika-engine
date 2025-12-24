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
		  m_usageState(usageState),
		  m_GPUAddress(resource->GetDesc().Height > 1 ? 0 : resource->GetGPUVirtualAddress()), //< Hacky way to detect if it's texture
		  m_isReady(false),
		  m_name(name)
	{
		// Assume MIP 0 since only one handle is provided
		m_descriptorHeapHandles[0] = descriptorHeapHandles;

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
		for (int i = 0; i < gMaxMipsAllowed; ++i)
		{
			m_descriptorHeapHandles[i] = other.m_descriptorHeapHandles[i];
		}

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
			for (int i = 0; i < gMaxMipsAllowed; ++i)
			{
				m_descriptorHeapHandles[i] = other.m_descriptorHeapHandles[i];
			}
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

		for (int frameIdx = 0; frameIdx < gFrameCount; frameIdx++)
		{
			for (int resourceViewType = 0; resourceViewType < ToInt(EResourceViewType::NUM_TYPES); resourceViewType++)
			{
				EResourceViewType viewType = static_cast<EResourceViewType>(resourceViewType);
				D3D12_DESCRIPTOR_HEAP_TYPE heapType = ViewTypeToHeapType(viewType);
				// TODO: We don't support dynamic deallocation of resource descriptors yet (CBV_SRV_UAV)
				if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
				{
					continue;
				}

				for (int i = 0; i < gMaxMipsAllowed; ++i)
				{
					const DescriptorHeapHandle& handle = m_descriptorHeapHandles[i].At(frameIdx, viewType);
					if (handle.IsValid())
					{
						gDescriptorHeapManager->FreeDescriptor(heapType, handle);
					}
				}
			}
		}

		m_resource = nullptr;
	}

	void GPUResource::SetupResourceStats()
	{
		m_sizeInBytes = GetResouceSize(m_resource);

		std::lock_guard lock(g_ResourceCreationMutex);
		Logger::Assert(!gResourcesMap.contains(m_name.c_str()), L"Attempting to register GPU resource that already exists");
		gResourcesMap[m_name.c_str()] = this;
	}

	void GPUResource::AddDescriptorHandle(const DescriptorHeapHandle& heapHandle, EResourceViewType resourceViewType,
	                                      uint32_t frameIdx, uint8_t mipIdx)
	{
		Logger::Assert(heapHandle.IsValid(),
			L"Attempting to create GPU resource with null descriptor heap element. Please provide a valid one in constructor.");

		m_descriptorHeapHandles[mipIdx].At(frameIdx, resourceViewType) = heapHandle;
	}

	const DescriptorHeapHandle& GPUResource::GetDescriptorHeapHandle(EResourceViewType resourceViewType, uint32_t frameIdx, uint8_t mipIdx) const
	{
		Logger::Assert(m_descriptorHeapHandles[mipIdx].Get(frameIdx, resourceViewType).IsValid(),
			L"Attempting to get heap element that doesn't exist in current resource.");
		return m_descriptorHeapHandles[mipIdx].Get(frameIdx, resourceViewType);
	}

	uint32_t GPUResource::GetIndexInRDH(EResourceViewType resourceViewType, uint8_t mipIdx) const
	{
		// TODO; Always index from frame 0, might be dangerous
		return m_descriptorHeapHandles[mipIdx].Get(0, resourceViewType).GetHeapIndex(); //< TODO: Consider unified handle
	}


	size_t GPUResource::GetSizeInBytes() const
	{
		return m_sizeInBytes;
	}

	ComPtr<ID3D12Resource> GPUResourceUtils::CreateUninitializedGPUBuffer(size_t alignedSize, LPCSTR name)
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
			D3D12_RESOURCE_STATE_COMMON, //< Should transition? Nothing seems to complain though...
			nullptr,
		IID_PPV_ARGS(&bufferResource)));

		NAME_D3D12_OBJECT_CUSTOM(bufferResource, name);

		return bufferResource;
	}
}