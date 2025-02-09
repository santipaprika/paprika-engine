#pragma once

#include <RHI/GPUResource.h>

namespace PPK
{
    namespace RHI
    {
        class VertexBuffer : public GPUResource
        {
        public:
            VertexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t vertexStride, uint32_t bufferSize);
            ~VertexBuffer() override = default;

            [[nodiscard]] const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vertexBufferView; };
            static ComPtr<ID3D12Resource> CreateIABufferResource(void* bufferData, uint32_t bufferSize, bool isIndexBuffer = false);
            static VertexBuffer* CreateVertexBuffer(void* vertexBufferData, uint32_t vertexBufferStride, uint32_t vertexBufferSize);

        private:
            D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        };

        class IndexBuffer : public GPUResource
        {
        public:
            IndexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t bufferSize);
            ~IndexBuffer() override = default;

            [[nodiscard]] const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_indexBufferView; }
            static IndexBuffer* CreateIndexBuffer(void* indexBufferData, uint32_t indexBufferSize);

        private:
            D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        };
    }
}