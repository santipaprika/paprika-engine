#include <Renderer.h>
#include <RHI/VertexBuffer.h>
#include <ApplicationHelper.h>

namespace PPK::RHI
{
	VertexBuffer::VertexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t vertexStride,
	                           uint32_t bufferSize, LPCWSTR name)
		: GPUResource(resource, DescriptorHeapElements{}, usageState, name)
	{
		m_GPUAddress = resource->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = vertexStride;
		m_vertexBufferView.SizeInBytes = bufferSize;
		m_vertexBufferView.BufferLocation = m_GPUAddress;
	}

	ComPtr<ID3D12Resource> VertexBuffer::CreateIABufferResource(void* bufferData, uint32_t bufferSize, bool isIndexBuffer)
	{
		ComPtr<ID3D12Resource> iaResource = CreateInitializedGPUResource(bufferData, bufferSize,
			isIndexBuffer ? D3D12_RESOURCE_STATE_INDEX_BUFFER : D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		return iaResource;
	}

	VertexBuffer* VertexBuffer::CreateVertexBuffer(void* vertexBufferData, uint32_t vertexStride,
		uint32_t vertexBufferSize)
	{
		ComPtr<ID3D12Resource> vbResource = CreateIABufferResource(vertexBufferData, vertexBufferSize);

		LPCWSTR name = L"VtxBufferMesh";
		NAME_D3D12_OBJECT_CUSTOM(vbResource, name);

		// Initialize the vertex buffer wrapper object containing GPU address and the vertex view.
		return new VertexBuffer(vbResource.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertexStride, vertexBufferSize, name);
	}

	IndexBuffer::IndexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t bufferSize, LPCWSTR name)
		: GPUResource(resource, DescriptorHeapElements{}, usageState, name)
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

		LPCWSTR name = L"IdxBufferMesh";
		NAME_D3D12_OBJECT_CUSTOM(ibResource, name);

		// Initialize the vertex buffer wrapper object containing GPU address and the vertex view.
		return new IndexBuffer(ibResource.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER, indexBufferSize, name);
	}
}
