#include <Renderer.h>
#include <RHI/ConstantBuffer.h>

namespace PPK::RHI
{
	ConstantBuffer::ConstantBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState,
	                                         uint32_t bufferSize, DescriptorHeapHandle constantBufferViewHandle)
		: GPUResource(resource, usageState)
	{
		m_GPUAddress = resource->GetGPUVirtualAddress();
		m_bufferSize = bufferSize;
		m_constantBufferViewHandle = constantBufferViewHandle;

		m_mappedBuffer = NULL;
		m_resource->Map(0, NULL, reinterpret_cast<void**>(&m_mappedBuffer));
	}

	ConstantBuffer::~ConstantBuffer()
	{
		m_resource->Unmap(0, NULL);
	}

	void ConstantBuffer::SetConstantBufferData(const void* bufferData, uint32_t bufferSize)
	{
		assert(bufferSize <= m_bufferSize);
		memcpy(m_mappedBuffer, bufferData, bufferSize);
	}

	ConstantBuffer* ConstantBuffer::CreateConstantBuffer(uint32_t bufferSize)
	{
		ID3D12Resource* constantBufferResource = NULL;
		const uint32_t alignedSize = (bufferSize / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT + 1) * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		
		D3D12_RESOURCE_DESC constantBufferDesc;
		constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		constantBufferDesc.Alignment = 0;
		constantBufferDesc.Width = alignedSize;
		constantBufferDesc.Height = 1;
		constantBufferDesc.DepthOrArraySize = 1;
		constantBufferDesc.MipLevels = 1;
		constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		constantBufferDesc.SampleDesc.Count = 1;
		constantBufferDesc.SampleDesc.Quality = 0;
		constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		
		D3D12_HEAP_PROPERTIES uploadHeapProperties;
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		uploadHeapProperties.CreationNodeMask = 0;
		uploadHeapProperties.VisibleNodeMask = 0;
		
		ThrowIfFailed(DX12Interface::Get()->GetDevice()->CreateCommittedResource(
			&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
			IID_PPV_ARGS(&constantBufferResource)));
		
		D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
		constantBufferViewDesc.BufferLocation = constantBufferResource->GetGPUVirtualAddress();
		constantBufferViewDesc.SizeInBytes = alignedSize;
		
		DescriptorHeapHandle constantBufferHeapHandle = GPUResourceManager::Get()->GetNewHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DX12Interface::Get()->GetDevice()->CreateConstantBufferView(&constantBufferViewDesc, constantBufferHeapHandle.GetCPUHandle());

		// TODO: This is probably better as reference
		ConstantBuffer* constantBuffer = new ConstantBuffer(constantBufferResource, D3D12_RESOURCE_STATE_GENERIC_READ,
			bufferSize, constantBufferHeapHandle);
		constantBuffer->SetIsReady(true);
		
		return constantBuffer;
	}
}
