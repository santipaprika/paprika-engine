#include <ApplicationHelper.h>
#include <cassert>
#include <Renderer.h>
#include <RHI/ConstantBuffer.h>

namespace PPK::RHI
{
	ConstantBuffer::ConstantBuffer(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState,
	                               uint32_t bufferSize, std::shared_ptr<DescriptorHeapElement> constantBufferViewElement)
		: GPUResource(resource, constantBufferViewElement, usageState)
	{
		//m_GPUAddress = resource->GetGPUVirtualAddress();
		m_bufferSize = bufferSize;

		m_mappedBuffer = NULL;
		m_resource->Map(0, NULL, reinterpret_cast<void**>(&m_mappedBuffer));
	}

	ConstantBuffer::~ConstantBuffer()
	{
		Logger::Info("REMOVING CB");
	}

	void ConstantBuffer::SetConstantBufferData(const void* bufferData, uint32_t bufferSize)
	{
		assert(bufferSize <= m_bufferSize);
		memcpy(m_mappedBuffer, bufferData, bufferSize);
	}

	ConstantBuffer* ConstantBuffer::CreateConstantBuffer(uint32_t bufferSize, LPCWSTR name,
	                                                     bool allowCpuWrites, const void* bufferData)
	{
		ComPtr<ID3D12Resource> constantBufferResource = nullptr;
		const uint32_t alignedSize = (bufferSize / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT + 1) *
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

		ComPtr<ID3D12Resource> stagingBufferResource = nullptr;
		ThrowIfFailed(gDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(alignedSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&stagingBufferResource)));

		if (!allowCpuWrites)
		{
			ThrowIfFailed(gDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(alignedSize),
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

				const ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCurrentCommandListReset();
				// This performs the memcpy through intermediate buffer
				UpdateSubresources<1>(commandList.Get(), constantBufferResource.Get(), stagingBufferResource.Get(), 0, 0, 1,
					&subresourceData);
				commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
					constantBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_GENERIC_READ));

				// Close the command list and execute it to begin the vertex buffer copy into
				// the default heap.
				ThrowIfFailed(commandList->Close());
				gRenderer->ExecuteCommandListOnce();
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
		gDevice->CreateConstantBufferView(&constantBufferViewDesc,
		                                                            constantBufferHeapElement->GetCPUHandle());

		// TODO: This is probably better as reference
		ConstantBuffer* constantBuffer = new ConstantBuffer(cbResource,
		                                                    D3D12_RESOURCE_STATE_GENERIC_READ,
		                                                    bufferSize, constantBufferHeapElement);
		constantBuffer->SetIsReady(true);

		return constantBuffer;
	}
}
