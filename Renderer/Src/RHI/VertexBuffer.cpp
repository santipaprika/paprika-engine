#include <RHI/VertexBuffer.h>
#include <Renderer.h>
#include <ApplicationHelper.h>

namespace PPK::RHI
{
	VertexBuffer::VertexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t vertexStride,
	                           uint32_t bufferSize)
		: GPUResource(resource, DescriptorHeapHandle::Null(), usageState)
	{
		m_GPUAddress = resource->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = vertexStride;
		m_vertexBufferView.SizeInBytes = bufferSize;
		m_vertexBufferView.BufferLocation = m_GPUAddress;
	}

	ComPtr<ID3D12Resource> VertexBuffer::CreateIABufferResource(void* bufferData, uint32_t bufferSize, bool isIndexBuffer)
	{
		ComPtr<ID3D12Resource> bufferUploadResource;
		ComPtr<ID3D12Resource> bufferResource;

		// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
		// the command list that references it has finished executing on the GPU.
		// We will flush the GPU at the end of this method to ensure the resource is not
		// prematurely destroyed.

		ThrowIfFailed(DX12Interface::Get()->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&bufferResource)));

		ThrowIfFailed(DX12Interface::Get()->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&bufferUploadResource)));

		// Copy data to the intermediate upload heap and then schedule a copy
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		const ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCurrentCommandListReset();
		// This performs the memcpy through intermediate buffer
		UpdateSubresources<1>(commandList.Get(), bufferResource.Get(), bufferUploadResource.Get(), 0, 0, 1,
			&subresourceData);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			bufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
			isIndexBuffer ? D3D12_RESOURCE_STATE_INDEX_BUFFER : D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(commandList->Close());
		gRenderer->ExecuteCommandListOnce();

		// Upload temp buffer will be released (and its GPU resource!) after leaving current scope, but
		// it's safe because ExecuteCommandListOnce already waits for the GPU command list to execute.
		return bufferResource.Get();
	}

	VertexBuffer* VertexBuffer::CreateVertexBuffer(void* vertexBufferData, uint32_t vertexStride,
		uint32_t vertexBufferSize)
	{
		ComPtr<ID3D12Resource> vbResource = CreateIABufferResource(vertexBufferData, vertexBufferSize);

		NAME_D3D12_OBJECT_CUSTOM(vbResource, L"VtxBufferMesh");

		// Initialize the vertex buffer wrapper object containing GPU address and the vertex view.
		return new VertexBuffer(vbResource.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertexStride, vertexBufferSize);
	}

	IndexBuffer::IndexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t bufferSize)
		: GPUResource(resource, DescriptorHeapHandle::Null(), usageState)
	{
		m_GPUAddress = resource->GetGPUVirtualAddress();
		m_indexBufferView.SizeInBytes = bufferSize;
		m_indexBufferView.BufferLocation = m_GPUAddress;
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	}

	IndexBuffer* IndexBuffer::CreateIndexBuffer(void* indexBufferData,
	                                            uint32_t indexBufferSize)
	{
		ComPtr<ID3D12Resource> ibResource = VertexBuffer::CreateIABufferResource(indexBufferData, indexBufferSize, true);

		NAME_D3D12_OBJECT_CUSTOM(ibResource, L"IdxBufferMesh");

		// Initialize the vertex buffer wrapper object containing GPU address and the vertex view.
		return new IndexBuffer(ibResource.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, indexBufferSize);
	}
}
