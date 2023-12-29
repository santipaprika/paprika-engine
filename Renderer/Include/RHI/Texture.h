#pragma once

#include <RHI/GPUResource.h>

namespace PPK
{
    class Renderer;

    namespace RHI
    {
        class Texture : public GPUResource
        {
        public:
            Texture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle depthStencilViewHandle);
            Texture(const Texture&) = default;
        	~Texture() override;

            //[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetTextureView() const { return m_textureView; }
            static std::shared_ptr<Texture> CreateTextureResource(uint32_t width, uint32_t height, LPCWSTR name = L"TextureResource");

        private:
            //D3D12_DEPTH_STENCIL_VIEW_DESC m_textureView;
        };
    }
}