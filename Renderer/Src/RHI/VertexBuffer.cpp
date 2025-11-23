#include <Renderer.h>
#include <RHI/VertexBuffer.h>
#include <ApplicationHelper.h>

namespace PPK::RHI
{
	VertexBuffer::VertexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t vertexStride,
	                           uint32_t bufferSize, LPCSTR name)
		: GPUResource(resource, usageState, name)
	{
		m_GPUAddress = resource->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = vertexStride;
		m_vertexBufferView.SizeInBytes = bufferSize;
		m_vertexBufferView.BufferLocation = m_GPUAddress;
	}

	VertexBuffer* VertexBuffer::CreateVertexBuffer(void* vertexBufferData, uint32_t vertexStride,
	                                               uint32_t vertexBufferSize, std::string name)
	{
		ComPtr<ID3D12Resource> vbResource = GPUResourceUtils::CreateUninitializedGPUBuffer(vertexBufferSize,
			name.c_str(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		
		VertexBuffer* vertexBuffer = new VertexBuffer(vbResource.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			vertexStride, vertexBufferSize, name.c_str());

		// Copy data to the intermediate upload heap and then schedule a copy
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = vertexBufferData;
		subresourceData.RowPitch = vertexBufferSize;
		subresourceData.SlicePitch = vertexBufferSize;
		gRenderer->SetBufferData(subresourceData, vertexBuffer);

		// Initialize the vertex buffer wrapper object containing GPU address and the vertex view.
		return vertexBuffer;
	}

	IndexBuffer::IndexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t bufferSize, LPCSTR name)
		: GPUResource(resource, usageState, name)
	{
		m_GPUAddress = resource->GetGPUVirtualAddress();
		m_indexBufferView.SizeInBytes = bufferSize;
		m_indexBufferView.BufferLocation = m_GPUAddress;
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	}

	IndexBuffer* IndexBuffer::CreateIndexBuffer(void* indexBufferData,
	                                            uint32_t indexBufferSize, std::string name)
	{
		ComPtr<ID3D12Resource> ibResource = GPUResourceUtils::CreateUninitializedGPUBuffer(indexBufferSize,
			name.c_str(), D3D12_RESOURCE_STATE_INDEX_BUFFER);

		IndexBuffer* indexBuffer = new IndexBuffer(ibResource.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, indexBufferSize, name.c_str());
		// Copy data to the intermediate upload heap and then schedule a copy
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = indexBufferData;
		subresourceData.RowPitch = indexBufferSize;
		subresourceData.SlicePitch = indexBufferSize;
		gRenderer->SetBufferData(subresourceData, indexBuffer);
		// Initialize the vertex buffer wrapper object containing GPU address and the vertex view.
		return indexBuffer;
	}
}
