#pragma once

#include <DirectXTex.h>
#include <RHI/GPUResource.h>
#include <d3dx12/d3dx12_core.h>

namespace DirectX
{
    struct Image;
	struct TexMetadata;
}

namespace PPK
{
    constexpr float g_clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

    namespace RHI
    {
        class Texture : public GPUResource
        {
        public:
            Texture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, const DescriptorHeapHandles& textureHeapHandles, LPCSTR name);
            Texture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, LPCSTR name);
            Texture(const Texture&) = default;
        	~Texture() override;
        };

        std::shared_ptr<Texture> CreateMSDepthTextureResource(uint32_t width, uint32_t height, LPCSTR name = "DepthTextureResource");
    	std::shared_ptr<Texture> CreateResolvedDepthTextureResource(uint32_t width, uint32_t height, LPCSTR name);

        std::shared_ptr<Texture> CreateTextureResource(LPCSTR name, const DirectX::ScratchImage* inputImage);
        std::shared_ptr<Texture> CreateTextureResource(D3D12_RESOURCE_DESC textureDesc, LPCSTR name = "TextureResource", const DirectX::ScratchImage* inputImage = nullptr, const
                                                       D3D12_CLEAR_VALUE& clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, g_clearColor));
    }
}