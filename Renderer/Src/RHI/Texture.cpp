#include <RHI/Texture.h>
#include <Renderer.h>
#include <ApplicationHelper.h>
#include <DirectXTex.h>

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
			// TODO: This should be according to the type, not all DSV. Leak on exit.
			GPUResourceManager::Get()->FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_descriptorHeapHandle);
		}
	}

	std::shared_ptr<Texture> Texture::CreateDepthTextureResource(uint32_t width, uint32_t height, LPCWSTR name)
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

		DescriptorHeapHandle textureDsvHeapHandle = GPUResourceManager::Get()->GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DEPTH_STENCIL_VIEW_DESC textureViewDesc = {};
		textureViewDesc.Format = texDesc.Format;
		textureViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		DX12Interface::Get()->GetDevice()->CreateDepthStencilView(textureResource.Get(), &textureViewDesc, textureDsvHeapHandle.GetCPUHandle());

		return std::make_shared<Texture>(textureResource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ, textureDsvHeapHandle);
	}

	std::shared_ptr<Texture> Texture::CreateTextureResource(DirectX::TexMetadata textureMetadata, LPCWSTR name, const DirectX::Image* inputImage)
	{
		const bool is3DTexture = textureMetadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D;
		D3D12_RESOURCE_DESC textureDesc{};
		textureDesc.Format = textureMetadata.format;
		textureDesc.Width = static_cast<uint32_t>(textureMetadata.width);
		textureDesc.Height = static_cast<uint32_t>(textureMetadata.height);
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = is3DTexture ? static_cast<uint16_t>(textureMetadata.depth) : static_cast<uint16_t>(textureMetadata.arraySize);
		textureDesc.MipLevels = static_cast<uint16_t>(textureMetadata.mipLevels);
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = is3DTexture ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.Alignment = 0;

		ComPtr<ID3D12Resource> textureResource = nullptr;
		ThrowIfFailed(DX12Interface::Get()->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			inputImage ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureResource)));

		NAME_D3D12_OBJECT_CUSTOM(textureResource, name);

		DescriptorHeapHandle textureSrvHeapHandle = GPUResourceManager::Get()->GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DX12Interface::Get()->GetDevice()->CreateShaderResourceView(textureResource.Get(), NULL, textureSrvHeapHandle.GetCPUHandle());

		// Upload input image if one was provided
		if (inputImage)
		{
			uint64_t textureMemorySize = 0;
			UINT numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];
			UINT64 rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MAX_TEXTURE_SUBRESOURCE_COUNT];
			const uint64_t numSubResources = textureDesc.MipLevels * textureDesc.DepthOrArraySize;
			DX12Interface::Get()->GetDevice()->GetCopyableFootprints(&textureDesc, 0, static_cast<uint32_t>(numSubResources), 0, layouts,
				numRows, rowSizesInBytes, &textureMemorySize);
			const uint32_t alignedSize = (textureMemorySize / D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT + 1) *
				D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
			ComPtr<ID3D12Resource> stagingTextureResource = nullptr;
			ThrowIfFailed(DX12Interface::Get()->GetDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(inputImage->slicePitch),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&stagingTextureResource)));

			// Copy data to the intermediate upload heap and then schedule a copy
			// from the upload heap to the default heap constant buffer.
			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = inputImage->pixels;
			subresourceData.RowPitch = inputImage->rowPitch;
			subresourceData.SlicePitch = inputImage->slicePitch;

			const ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCurrentCommandListReset();
			// This performs the memcpy through intermediate buffer
			// TODO: Should be >1 subresources for mips/slices
			UpdateSubresources<1>(commandList.Get(), textureResource.Get(), stagingTextureResource.Get(), 0, 0, 1,
				&subresourceData);

			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_GENERIC_READ));

			// Close the command list and execute it to begin the input texture copy into
			// the default heap.
			ThrowIfFailed(commandList->Close());
			gRenderer->ExecuteCommandListOnce();

			// TODO: Code to handle mips/slices/depth
			//for (uint32_t arrayIndex = 0; arrayIndex < (is3DTexture ? 1 : textureDesc.DepthOrArraySize); arrayIndex++)
			//{
			//	for (uint32_t mipIndex = 0; mipIndex < textureDesc.MipLevels; mipIndex++)
			//	{
			//		const uint32_t subResourceIndex = mipIndex + (arrayIndex * textureDesc.MipLevels);
			//		const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subResourceLayout = layouts[subResourceIndex];
			//		const uint32_t subResourceHeight = numRows[subResourceIndex];
			//		const uint32_t subResourcePitch = subResourceLayout.Footprint.RowPitch; // May need to align explicitly with D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
			//		const uint32_t subResourceDepth = subResourceLayout.Footprint.Depth;
			//		uint8_t* destinationSubResourceMemory = uploadMemory + subResourceLayout.Offset;

			//		for (uint32_t sliceIndex = 0; sliceIndex < (is3DTexture ? textureDesc.DepthOrArraySize : 1); sliceIndex++)
			//		{
			//			const DirectX::Image* subImage = imageData->GetImage(mipIndex, arrayIndex, sliceIndex);
			//			const uint8_t* sourceSubResourceMemory = subImage->pixels;

			//			for (uint32_t height = 0; height < subResourceHeight; height++)
			//			{
			//				memcpy(destinationSubResourceMemory, sourceSubResourceMemory, MathHelper::Min(subResourcePitch, subImage->rowPitch));
			//				destinationSubResourceMemory += subResourcePitch;
			//				sourceSubResourceMemory += subImage->rowPitch;
			//			}
			//		}
			//	}
			//}
		}
		return std::make_shared<Texture>(textureResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, textureSrvHeapHandle);


	}
}
