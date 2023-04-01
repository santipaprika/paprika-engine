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

        	static VertexBuffer* CreateVertexBuffer(void* vertexData, uint32_t vertexStride, uint32_t vertexBufferSize, Renderer& renderer);

            [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_vertexBufferView; }

        private:
            D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        };
    }
}