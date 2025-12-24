#include <Renderer.h>
#include <RHI/Texture.h>
#include <ApplicationHelper.h>
#include <DirectXTex.h>

namespace PPK::RHI
{
	Texture::Texture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, const DescriptorHeapHandles& textureHeapHandles, LPCSTR name)
		: GPUResource(resource, textureHeapHandles, usageState, name)
	{
		//m_GPUAddress = resource->GetGPUVirtualAddress();
	}

	Texture::Texture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, LPCSTR name)
		: GPUResource(resource, usageState, name) // TODO: Is Texture really needed? Could be just utils funcs right now 
	{
	}

	Texture::~Texture()
	{
		Logger::Verbose(("REMOVING Texture " + std::string(m_name)).c_str());
	}

	std::shared_ptr<Texture> CreateMSDepthTextureResource(uint32_t width, uint32_t height, LPCSTR name)
	{
		ComPtr<ID3D12Resource> textureResource;
		ComPtr<ID3D12Resource> resolvedMSAAResource;

		D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R24G8_TYPELESS, // This triggers a validation layer warning because we can't provide optimized default value, but needed for SRV usage
			width,
			height);
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		texDesc.MipLevels = 1;

		// TODO: For MSAA, resource format can't be typeless. We'd need to copy texture later if we want to use it as SRV.
		// Use MSAA and check supported quality modes
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaLevels;
		msaaLevels.Format = DXGI_FORMAT_R24G8_TYPELESS;
		msaaLevels.SampleCount = gMSAA ? gMSAACount : 1;
		msaaLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		ThrowIfFailed(gDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaLevels, sizeof(msaaLevels)));
		texDesc.SampleDesc.Count = gMSAA ? gMSAACount : 1;
		texDesc.SampleDesc.Quality = gMSAA ? msaaLevels.NumQualityLevels - 1 : 1; // Max quality

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

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = gMSAA ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.Shader4ComponentMapping =  D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = gMSAA ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;

		DescriptorHeapHandles descriptorHeapHandles;
		descriptorHeapHandles.At(0, EResourceViewType::DSV) = gDescriptorHeapManager->GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		gDevice->CreateDepthStencilView(textureResource.Get(), &dsvDesc, descriptorHeapHandles.At(0, EResourceViewType::DSV).GetCPUHandle());
		for (int i = 0; i < gFrameCount; i++)
		{
			ShaderDescriptorHeap* resourceDescriptorHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i);
			descriptorHeapHandles.At(i, EResourceViewType::SRV) = resourceDescriptorHeap->GetHeapLocationNewHandle(HeapLocation::TEXTURES);
			gDevice->CreateShaderResourceView(textureResource.Get(), &srvDesc, descriptorHeapHandles.At(i, EResourceViewType::SRV).GetCPUHandle());
		}

		return std::make_shared<Texture>(textureResource.Get(),
		                                 D3D12_RESOURCE_STATE_DEPTH_WRITE,
		                                 descriptorHeapHandles, name);
	}

	std::shared_ptr<Texture> CreateTextureResource(DirectX::TexMetadata textureMetadata, LPCSTR name, const DirectX::Image* inputImage)
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

	std::shared_ptr<Texture> CreateTextureResource(D3D12_RESOURCE_DESC textureDesc, LPCSTR name, const DirectX::Image* inputImage, const D3D12_CLEAR_VALUE& clearValue)
	{
		// Create a named variable for the heap properties
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

		Logger::Assert((textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == 0 || clearValue.Format == textureDesc.Format,
			("Attempting to create texture" + std::string(name) + " with mismatching format between texture desc and optimized clear value. Desc: ").c_str());
		
		D3D12_RESOURCE_STATES usageState = inputImage ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_RENDER_TARGET;
		ComPtr<ID3D12Resource> textureResource = nullptr;
		ThrowIfFailed(gDevice->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc, // TODO: With DESC1 we can use sampler feedback for smart mipping
			usageState,
			(textureDesc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)) ? &clearValue : nullptr,
			IID_PPV_ARGS(&textureResource)));

		NAME_D3D12_OBJECT_CUSTOM(textureResource, name);
		Logger::Verbose(("CREATING heap element for texture " + std::string(name)).c_str());

		std::shared_ptr<Texture> texture = std::make_shared<Texture>(textureResource.Get(), usageState, name);

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

			// Copy data to the intermediate upload heap and then schedule a copy
			// from the upload heap to the default heap constant buffer.
			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = inputImage->pixels;
			subresourceData.RowPitch = inputImage->rowPitch;
			subresourceData.SlicePitch = inputImage->slicePitch;

			gRenderer->SetBufferData(subresourceData, texture.get());
			// Upload temp buffer will be released (and its GPU resource!) after leaving current scope, but
			// it's safe because ExecuteCommandListOnce already waits for the GPU command list to execute.


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
			{
				// Assume that all textures not initialized with disk data will be rendered at some point (RTV)
				DescriptorHeapHandle handle = gDescriptorHeapManager->GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				gDevice->CreateRenderTargetView(textureResource.Get(), NULL, handle.GetCPUHandle());
				texture->AddDescriptorHandle(handle, EResourceViewType::RTV, 0, /* MIP */ 0); //< TODO: Only MIP 0 has valid RTV
			}

			for (int mipIdx = 0; mipIdx < textureDesc.MipLevels; mipIdx++)
			{
				// Assume that all textures not initialized with disk data will be used as UAV at some point
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
				uavDesc.Format = textureDesc.Format;
				if (textureDesc.SampleDesc.Count > 1)
				{
					// MS texture
					uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					uavDesc.Texture2D.MipSlice = mipIdx;
					uavDesc.Texture2D.PlaneSlice = 0;
				}
				for (int frameIdx = 0; frameIdx < gFrameCount; frameIdx++)
				{
					ShaderDescriptorHeap* resourceDescriptorHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
					DescriptorHeapHandle handle = resourceDescriptorHeap->GetHeapLocationNewHandle(HeapLocation::TEXTURES);
					gDevice->CreateUnorderedAccessView(textureResource.Get(), nullptr, &uavDesc, handle.GetCPUHandle());
					texture->AddDescriptorHandle(handle, EResourceViewType::UAV, frameIdx, mipIdx);
				}
			}
		}

		
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = textureDesc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = textureDesc.SampleDesc.Count > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;

		for (int i = 0; i < gFrameCount; i++)
		{
			ShaderDescriptorHeap* resourceDescriptorHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i);
			DescriptorHeapHandle handle = resourceDescriptorHeap->GetHeapLocationNewHandle(HeapLocation::TEXTURES);
			texture->AddDescriptorHandle(handle, EResourceViewType::SRV, i);
			gDevice->CreateShaderResourceView(textureResource.Get(), textureDesc.SampleDesc.Count > 1 ? &srvDesc : nullptr, handle.GetCPUHandle());
		}

		return texture;
	}
}
