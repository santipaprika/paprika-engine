#include <ApplicationHelper.h>
#include <cassert>
#include <Renderer.h>
#include <RHI/ConstantBuffer.h>

namespace PPK::RHI
{
	ConstantBuffer::ConstantBuffer(): m_mappedBuffer(nullptr), m_bufferSize(0)
	{
	}

	ConstantBuffer::ConstantBuffer(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState,
	                               uint32_t bufferSize, std::shared_ptr<DescriptorHeapElement> constantBufferViewElement, LPCSTR name)
		: GPUResource(resource, constantBufferViewElement, usageState, name)
	{
		//m_GPUAddress = resource->GetGPUVirtualAddress();
		m_bufferSize = bufferSize;

		m_mappedBuffer = NULL;
		CD3DX12_RANGE readRange(0, 0); // We won't read from this resource on CPU. TODO: should be driven by flag maybe?
		m_resource->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedBuffer));
	}

	ConstantBuffer::ConstantBuffer(ConstantBuffer&& other) noexcept: GPUResource(std::move(other))
	{
		m_mappedBuffer = other.m_mappedBuffer;
		other.m_mappedBuffer = nullptr;

		m_bufferSize = other.m_bufferSize;
	}

	ConstantBuffer& ConstantBuffer::operator=(ConstantBuffer&& other) noexcept
	{
		if (this != &other)
		{
			m_mappedBuffer = other.m_mappedBuffer;
			other.m_mappedBuffer = nullptr;
	
			m_bufferSize = other.m_bufferSize;
		}

		GPUResource::operator=(GPUResource(std::forward<GPUResource>(other)));
	
		return *this;
	}

	ConstantBuffer::~ConstantBuffer()
	{
		Logger::Info(("REMOVING CB " + std::string(m_name)).c_str());
	}

	void ConstantBuffer::SetConstantBufferData(const void* bufferData, uint32_t bufferSize)
	{
		assert(bufferSize <= m_bufferSize);
		memcpy(m_mappedBuffer, bufferData, bufferSize);
	}

	ConstantBuffer CreateConstantBuffer(uint32_t bufferSize, LPCSTR name,
	                                    bool allowCpuWrites, const void* bufferData, uint32_t alignment)
	{
		ComPtr<ID3D12Resource> constantBufferResource = nullptr;
		const uint32_t alignedSize = (bufferSize / alignment + 1) * alignment;

		// Create a named variable for the heap properties
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

		// Create a named variable for the resource description
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);

		ComPtr<ID3D12Resource> stagingBufferResource = nullptr;
		ThrowIfFailed(gDevice->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&stagingBufferResource)));

		if (!allowCpuWrites)
		{
			ThrowIfFailed(gDevice->CreateCommittedResource(
				&defaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				bufferData ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
			IID_PPV_ARGS(&constantBufferResource)));

			if (bufferData)
			{
				// Copy data to the intermediate upload heap and then schedule a copy
				// from the upload heap to the default heap constant buffer.
				D3D12_SUBRESOURCE_DATA subresourceData = {};
				subresourceData.pData = bufferData;
				subresourceData.RowPitch = bufferSize;
				subresourceData.SlicePitch = subresourceData.RowPitch;

				CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
					constantBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_GENERIC_READ);
				GPUResourceUtils::UpdateSubresourcesImmediately(stagingBufferResource, constantBufferResource, subresourceData, transition);

				// Upload temp buffer will be released (and its GPU resource!) after leaving this function, but
				// it's safe because ExecuteCommandListOnce already waits for the GPU command list to execute.
			}
			else
			{
				Logger::Warning("Creating buffer in default heap without providing initial data. Is this intended?");
			}

			NAME_D3D12_OBJECT_CUSTOM(constantBufferResource, name);
		}
		else
		{
			NAME_D3D12_OBJECT_CUSTOM(stagingBufferResource, name);
		}

		ComPtr<ID3D12Resource> cbResource = allowCpuWrites ? stagingBufferResource : constantBufferResource;
		
		D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
		constantBufferViewDesc.SizeInBytes = alignedSize;
		constantBufferViewDesc.BufferLocation = cbResource->GetGPUVirtualAddress();

		std::shared_ptr<DescriptorHeapElement> constantBufferHeapElement = std::make_shared<DescriptorHeapElement>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		Logger::Info(("CREATING heap element for buffer " + std::string(name)).c_str());
		gDevice->CreateConstantBufferView(&constantBufferViewDesc,constantBufferHeapElement->GetCPUHandle());

		ConstantBuffer constantBuffer = std::move(ConstantBuffer(cbResource,
		                                               D3D12_RESOURCE_STATE_GENERIC_READ,
		                                               bufferSize, constantBufferHeapElement, name));
		constantBuffer.SetIsReady(true);

		return std::move(constantBuffer);
	}
}
