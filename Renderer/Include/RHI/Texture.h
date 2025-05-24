#pragma once

#include <RHI/GPUResource.h>

#define MAX_TEXTURE_SUBRESOURCE_COUNT 6

namespace DirectX
{
    struct Image;
	struct TexMetadata;
}

namespace PPK
{
    namespace RHI
    {
        class Texture : public GPUResource
        {
        public:
            Texture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, const DescriptorHeapElements& textureHeapElements, LPCWSTR name);
            Texture(const Texture&) = default;
        	~Texture() override;

            //[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetTextureView() const { return m_textureView; }

        private:
            //D3D12_DEPTH_STENCIL_VIEW_DESC m_textureView;
        };

        std::shared_ptr<Texture> CreateDepthTextureResource(uint32_t width, uint32_t height, LPCWSTR name = L"DepthTextureResource");
        std::shared_ptr<Texture> CreateTextureResource(DirectX::TexMetadata textureMetadata, LPCWSTR name = L"TextureResource", const DirectX::Image* inputImage = nullptr);
        std::shared_ptr<Texture> CreateTextureResource(D3D12_RESOURCE_DESC textureDesc, LPCWSTR name = L"DiskTextureResource", const DirectX::Image* inputImage = nullptr);
    }

    constexpr float g_clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
}