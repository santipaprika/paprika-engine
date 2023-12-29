#include <RHI/Texture.h>
#include <Renderer.h>
#include <ApplicationHelper.h>

namespace PPK::RHI
{
	Texture::Texture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle depthStencilViewHandle)
		: GPUResource(resource, depthStencilViewHandle, usageState)
	{
		//m_GPUAddress = resource->GetGPUVirtualAddress();
	}

	Texture::~Texture()
	{
		Logger::Info("REMOVING Texture");
		if (m_resource.Get())
		{
			m_resource->Unmap(0, NULL);
			GPUResourceManager::Get()->FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_descriptorHeapHandle);
		}
	}

	std::shared_ptr<Texture> Texture::CreateTextureResource(uint32_t width, uint32_t height, LPCWSTR name)
	{
		ComPtr<ID3D12Resource> textureResource;

		D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			width,
			height);
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		ThrowIfFailed(DX12Interface::Get()->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&CD3DX12_CLEAR_VALUE(texDesc.Format, 1.f, 0),
			IID_PPV_ARGS(&textureResource)));

		NAME_D3D12_OBJECT_CUSTOM(textureResource, name);

		DescriptorHeapHandle textureHeapHandle = GPUResourceManager::Get()->GetNewHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DEPTH_STENCIL_VIEW_DESC textureViewDesc = {};
		textureViewDesc.Format = texDesc.Format;
		textureViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		DX12Interface::Get()->GetDevice()->CreateDepthStencilView(textureResource.Get(), &textureViewDesc, textureHeapHandle.GetCPUHandle());

		return std::make_shared<Texture>(textureResource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ, textureHeapHandle);
	}
}
