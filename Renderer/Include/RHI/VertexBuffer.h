#pragma once

#include <RHI/GPUResource.h>

namespace PPK
{
    class Renderer;

    namespace RHI
    {
        class VertexBuffer : public GPUResource
        {
        public:
            VertexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t vertexStride, uint32_t bufferSize);
            ~VertexBuffer() override = default;
        	
            [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_vertexBufferView; }
            static ComPtr<ID3D12Resource> CreateIABufferResource(void* bufferData, uint32_t bufferSize, Renderer& renderer);
            static VertexBuffer* CreateVertexBuffer(void* vertexBufferData, uint32_t vertexBufferStride, uint32_t vertexBufferSize, Renderer& renderer);

        private:
            D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        };

        class IndexBuffer : public GPUResource
        {
        public:
            IndexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t bufferSize);
            ~IndexBuffer() override = default;

            [[nodiscard]] D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_indexBufferView; }
            static IndexBuffer* CreateIndexBuffer(void* indexBufferData, uint32_t indexBufferSize, Renderer& renderer);

        private:
            D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        };
    }
}