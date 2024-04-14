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
            Texture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle depthStencilViewHandle);
            Texture(const Texture&) = default;
        	~Texture() override;

            //[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetTextureView() const { return m_textureView; }
            static std::shared_ptr<Texture> CreateDepthTextureResource(uint32_t width, uint32_t height, LPCWSTR name = L"DepthTextureResource");
            static std::shared_ptr<Texture> CreateTextureResource(DirectX::TexMetadata textureMetadata, LPCWSTR name = L"TextureResource", const DirectX::Image* inputImage = nullptr);

        private:
            //D3D12_DEPTH_STENCIL_VIEW_DESC m_textureView;
        };
    }
}