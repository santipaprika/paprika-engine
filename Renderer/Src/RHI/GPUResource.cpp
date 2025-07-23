#include <ApplicationHelper.h>
#include <Renderer.h>
#include <RHI/GPUResource.h>
#include <Logger.h>

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
	
	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource, const DescriptorHeapElements& descriptorHeapElements,
	                         D3D12_RESOURCE_STATES usageState, const std::string& name)
		: m_resource(resource),
		  m_descriptorHeapElements(descriptorHeapElements), // TODO: Add descriptors to memory report as well
		  m_usageState(usageState),
		  m_GPUAddress(resource->GetDesc().Height > 1 ? 0 : resource->GetGPUVirtualAddress()), //< Hacky way to detect if it's texture
		  m_isReady(false),
		  m_name(name)
	{
		Logger::Assert(resource, L"Attempting to initialize GPU resource proxy with NULL resource.");

		gResourcesMap[name.c_str()] = this;
		m_sizeInBytes = GetResouceSize(resource);
	}

	GPUResource::GPUResource(ComPtr<ID3D12Resource> resource,
		std::shared_ptr<DescriptorHeapElement> descriptorHeapElement, D3D12_RESOURCE_STATES usageState,
		const std::string& name)
		: m_resource(resource),
		  m_usageState(usageState),
		  m_GPUAddress(resource->GetDesc().Height > 1 ? 0 : resource->GetGPUVirtualAddress()), //< Hacky way to detect if it's texture
		  m_isReady(false),
		  m_name(name)
	{
		if (descriptorHeapElement)
		{
			Logger::Assert(descriptorHeapElement != nullptr && descriptorHeapElement->IsValid(),
				L"Attempting to create GPU resource with null descriptor heap element. Please provide a valid one in constructor.");
			m_descriptorHeapElements[descriptorHeapElement->GetHeapType()] = descriptorHeapElement;
		}

		m_sizeInBytes = GetResouceSize(resource);
		gResourcesMap[name.c_str()] = this;
	}

	GPUResource::GPUResource(GPUResource&& other) noexcept
	{
		m_resource = other.m_resource;
		other.m_resource = nullptr; // Don't delete since it will be used by copied moved object

		m_GPUAddress = other.m_GPUAddress;
		m_usageState = other.m_usageState;
		m_isReady = other.m_isReady;
		m_descriptorHeapElements = other.m_descriptorHeapElements;

		m_name = other.m_name;
		m_sizeInBytes = other.m_sizeInBytes;

		other.m_resource = nullptr;

		gResourcesMap[m_name] = this;

		Logger::Info(("Moving resource " + std::string(m_name)).c_str());
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
			m_descriptorHeapElements = other.m_descriptorHeapElements;

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
			m_resource->Unmap(0, NULL);
			// Assuming single thread accesses to gResourcesMap
			gResourcesMap.erase(m_name);
		}

		m_resource = nullptr;
	}

	std::shared_ptr<DescriptorHeapElement> GPUResource::GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const
	{
		Logger::Assert(m_descriptorHeapElements[static_cast<int>(heapType)] != nullptr && m_descriptorHeapElements[static_cast<int>(heapType)]->IsValid(),
			L"Attempting to get heap element that doesn't exist in current resource.");
		return m_descriptorHeapElements[static_cast<int>(heapType)];
	}

	void GPUResource::CopyDescriptorsToShaderHeap(D3D12_CPU_DESCRIPTOR_HANDLE currentCBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE heapType) const
	{
		gDevice->CopyDescriptorsSimple(1, currentCBVHandle, GetDescriptorHeapElement(heapType)->GetCPUHandle(), heapType);
	}

	ComPtr<ID3D12Resource> GPUResource::CreateInitializedGPUResource(const void* data, size_t dataSize, D3D12_RESOURCE_STATES outputState)
	{
		ComPtr<ID3D12Resource> bufferUploadResource;
		ComPtr<ID3D12Resource> bufferResource;

		// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
		// the command list that references it has finished executing on the GPU.
		// We will flush the GPU at the end of this method to ensure the resource is not
		// prematurely destroyed.

		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

		ThrowIfFailed(gDevice->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&bufferResource)));

		ThrowIfFailed(gDevice->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&bufferUploadResource)));

		// Copy data to the intermediate upload heap and then schedule a copy
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = data;
		subresourceData.RowPitch = dataSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(bufferResource.Get(),D3D12_RESOURCE_STATE_COPY_DEST, outputState);
		GPUResourceUtils::UpdateSubresourcesImmediately(bufferUploadResource, bufferResource, subresourceData, transition);

		// Upload temp buffer will be released (and its GPU resource!) after leaving current scope, but
		// it's safe because ExecuteCommandListOnce already waits for the GPU command list to execute.
		
		return bufferResource;
	}

	void GPUResource::TransitionTo(ComPtr<ID3D12GraphicsCommandList4> commandList, D3D12_RESOURCE_STATES destState)
	{
		if (destState == GetUsageState())
		{
			return;
		}

		// Indicate that the output of the base pass will be used as a PS resource now.
		const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(),
			GetUsageState(), destState);
		commandList->ResourceBarrier(1, &barrier);

		m_usageState = destState;
	}

	size_t GPUResource::GetSizeInBytes() const
	{
		return m_sizeInBytes;
	}

	void GPUResourceUtils::UpdateSubresourcesImmediately(ComPtr<ID3D12Resource> uploadResource,
	                                                     ComPtr<ID3D12Resource> resource,
	                                                     D3D12_SUBRESOURCE_DATA subresourceData,
	                                                     CD3DX12_RESOURCE_BARRIER transitionBarrier)
	{
		std::scoped_lock lock(g_ResourceCreationMutex);

		const ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCurrentCommandListReset();
		// This performs the memcpy through intermediate buffer
		// TODO: Should be >1 subresources for mips/slices
		UpdateSubresources<1>(commandList.Get(), resource.Get(), uploadResource.Get(), 0, 0, 1,
		                      &subresourceData);
		commandList->ResourceBarrier(1, &transitionBarrier);

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(commandList->Close());
		gRenderer->ExecuteCommandListOnce();
	}
}