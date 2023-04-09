#include <RHI/VertexBuffer.h>
#include <Renderer.h>
#include <ApplicationHelper.h>

namespace PPK::RHI
{
	VertexBuffer::VertexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t vertexStride,
	                           uint32_t bufferSize)
		: GPUResource(resource, usageState)
	{
		m_GPUAddress = resource->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = vertexStride;
		m_vertexBufferView.SizeInBytes = bufferSize;
		m_vertexBufferView.BufferLocation = m_GPUAddress;
	}

	VertexBuffer* VertexBuffer::CreateVertexBuffer(void* vertexData, uint32_t vertexStride, uint32_t vertexBufferSize,
	                                               Renderer& renderer)
	{
		ComPtr<ID3D12Resource> vertexBufferUploadResource;
		ComPtr<ID3D12Resource> vertexBufferResource;

		// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
		// the command list that references it has finished executing on the GPU.
		// We will flush the GPU at the end of this method to ensure the resource is not
		// prematurely destroyed.

		ThrowIfFailed(renderer.GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&vertexBufferResource)));
		
		ThrowIfFailed(renderer.GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBufferUploadResource)));
		
		NAME_D3D12_OBJECT_CUSTOM(vertexBufferResource, "Vtx_Buffer_Mesh");
		
		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA vertexSubresourceData = {};
		vertexSubresourceData.pData = vertexData;
		vertexSubresourceData.RowPitch = vertexBufferSize;
		vertexSubresourceData.SlicePitch = vertexSubresourceData.RowPitch;
		
		const ComPtr<ID3D12GraphicsCommandList4> commandList = renderer.GetCurrentCommandListReset();
		// This performs the memcpy through intermediate buffer
		UpdateSubresources<1>(commandList.Get(), vertexBufferResource.Get(), vertexBufferUploadResource.Get(), 0, 0, 1,
			&vertexSubresourceData);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			vertexBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Initialize the vertex buffer wrapper object containing GPU address and the vertex view.
		VertexBuffer* vertexBuffer = new VertexBuffer(vertexBufferResource.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertexStride, vertexBufferSize);

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(commandList->Close());
		renderer.ExecuteCommandListOnce();

		return vertexBuffer;
	}
}
