#include <Renderer.h>
#include <RHI/Texture.h>
#include <ApplicationHelper.h>
#include <DirectXTex.h>

namespace PPK::RHI
{
	Texture::Texture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, const DescriptorHeapElements& textureHeapElements, LPCWSTR name)
		: GPUResource(resource, textureHeapElements, usageState, name)
	{
		//m_GPUAddress = resource->GetGPUVirtualAddress();
	}

	Texture::~Texture()
	{
		Logger::Info((L"REMOVING Texture " + std::wstring(m_name)).c_str());
	}

	std::shared_ptr<Texture> CreateDepthTextureResource(uint32_t width, uint32_t height, LPCWSTR name)
	{
		ComPtr<ID3D12Resource> textureResource;

		D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R24_UNORM_X8_TYPELESS, // This triggers a validation layer warning because we can't provide optimized default value, but needed for SRV usage
			width,
			height);
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		// TODO: For MSAA, resource format can't be typeless. We'd need to copy texture later if we want to use it as SRV.
		// Use MSAA and check supported quality modes
		// D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaLevels;
		// msaaLevels.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Replace with your render target format.
		// msaaLevels.SampleCount = gMSAA ? gMSAACount : 1; // Replace with your sample count.
		// msaaLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		// ThrowIfFailed(gDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaLevels, sizeof(msaaLevels)));
		// texDesc.SampleDesc.Count = gMSAA ? gMSAACount : 1;
		// texDesc.SampleDesc.Quality = gMSAA ? msaaLevels.NumQualityLevels - 1 : 1; // Max quality
		texDesc.MipLevels = 1;

		// Create a named variable for the heap properties
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

		ThrowIfFailed(gDevice->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			nullptr,
			IID_PPV_ARGS(&textureResource)));

		NAME_D3D12_OBJECT_CUSTOM(textureResource, name);

		DescriptorHeapElements descriptorHeapElements;
		descriptorHeapElements[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = std::make_shared<DescriptorHeapElement>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DEPTH_STENCIL_VIEW_DESC textureViewDesc = {};
		textureViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		textureViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		gDevice->CreateDepthStencilView(textureResource.Get(), &textureViewDesc, descriptorHeapElements[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUHandle());

		textureViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		std::shared_ptr<DescriptorHeapElement> textureSrvHeapElement = std::make_shared<DescriptorHeapElement>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		gDevice->CreateShaderResourceView(textureResource.Get(), NULL, textureSrvHeapElement->GetCPUHandle());
		descriptorHeapElements[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = textureSrvHeapElement;

		return std::make_shared<Texture>(textureResource.Get(),
		                                 D3D12_RESOURCE_STATE_DEPTH_WRITE,
		                                 descriptorHeapElements, name);
	}

	std::shared_ptr<Texture> CreateTextureResource(DirectX::TexMetadata textureMetadata, LPCWSTR name, const DirectX::Image* inputImage)
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

		return CreateTextureResource(textureDesc, name, inputImage);
	}

	std::shared_ptr<Texture> CreateTextureResource(D3D12_RESOURCE_DESC textureDesc, LPCWSTR name, const DirectX::Image* inputImage, const D3D12_CLEAR_VALUE& clearValue)
	{
		// Create a named variable for the heap properties
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

		Logger::Assert((textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == 0 || clearValue.Format == textureDesc.Format,
			(L"Attempting to create texture" + std::wstring(name) + L" with mismatching format between texture desc and optimized clear value. Desc: ").c_str());
		
		D3D12_RESOURCE_STATES usageState = inputImage ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_RENDER_TARGET;
		ComPtr<ID3D12Resource> textureResource = nullptr;
		ThrowIfFailed(gDevice->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc, // TODO: With DESC1 we can use sampler feedback for smart mipping
			usageState,
			(textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ? &clearValue : nullptr,
			IID_PPV_ARGS(&textureResource)));

		NAME_D3D12_OBJECT_CUSTOM(textureResource, name);
		Logger::Info((L"CREATING heap element for texture " + std::wstring(name)).c_str());


		std::shared_ptr<DescriptorHeapElement> textureSrvHeapElement = std::make_shared<DescriptorHeapElement>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		gDevice->CreateShaderResourceView(textureResource.Get(), NULL, textureSrvHeapElement->GetCPUHandle());

		DescriptorHeapElements descriptorHeapElements;
		descriptorHeapElements[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = textureSrvHeapElement;


		// Upload input image if one was provided
		if (inputImage)
		{
			uint64_t textureMemorySize = 0;
			UINT numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];
			UINT64 rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MAX_TEXTURE_SUBRESOURCE_COUNT];
			const uint64_t numSubResources = textureDesc.MipLevels * textureDesc.DepthOrArraySize;
			gDevice->GetCopyableFootprints(&textureDesc, 0, static_cast<uint32_t>(numSubResources), 0, layouts,
				numRows, rowSizesInBytes, &textureMemorySize);
			const uint32_t alignedSize = (textureMemorySize / D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT + 1) * D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
			CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(inputImage->slicePitch);
			ComPtr<ID3D12Resource> stagingTextureResource = nullptr;
			ThrowIfFailed(gDevice->CreateCommittedResource(
				&uploadHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
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
			UpdateSubresources<1>(commandList.Get(), textureResource.Get(),
				stagingTextureResource.Get(), 0, 0, 1, &subresourceData);

			usageState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(textureResource.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, usageState);
			commandList->ResourceBarrier(1, &transition);

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
		else
		{
			// Assume that all textures not initialized with disk data will be rendered at some point (RTV)
			std::shared_ptr<DescriptorHeapElement> textureRtvHeapElement = std::make_shared<DescriptorHeapElement>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			gDevice->CreateRenderTargetView(textureResource.Get(), NULL, textureRtvHeapElement->GetCPUHandle());
			descriptorHeapElements[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = textureRtvHeapElement;
		}

		return std::make_shared<Texture>(textureResource.Get(), usageState, descriptorHeapElements, name);
	}
}
