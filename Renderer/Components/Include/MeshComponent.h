#pragma once

#include <Material.h>
#include <RHI/VertexBuffer.h>

#include <SimpleMath.h>
#include <RHI/ConstantBuffer.h>
#include <RHI/Texture.h> // For hardcoded albedo, remove

using namespace DirectX::SimpleMath;

namespace PPK
{
	struct RenderContext;
	class TransformComponent;

	// Low level interpretation of mesh which interacts with RHI and sets up the required buffers
	class MeshComponent
	{
	public:
        struct MeshBuildData
        {
			uint32_t m_nVertices;
			uint32_t m_nIndices;

			std::vector<uint32_t> m_indices;
            std::vector<float> m_vertices;
            std::vector<float> m_normals;
            std::vector<float> m_uvs;
            std::vector<uint32_t> m_colors;
        };

		struct ObjectData
		{
			Matrix transform;
		};

        explicit MeshComponent(const Material& material, RHI::ConstantBuffer&& constantBuffer,
        	RHI::VertexBuffer* vertexBuffer, uint32_t vertexCount, RHI::IndexBuffer* indexBuffer, uint32_t indexCount);
		~MeshComponent();
        MeshComponent(MeshComponent&& other) noexcept;

		struct Vertex
		{
			Vector3 position;
			Vector2 uv;
			Vector3 normal;
			Vector4 color;
		};

        [[nodiscard]] RHI::VertexBuffer* GetVertexBuffer() const { return m_vertexBuffer; };
        [[nodiscard]] const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vertexBuffer->GetVertexBufferView(); };
		[[nodiscard]] RHI::IndexBuffer* GetIndexBuffer() const { return m_indexBuffer; };
		[[nodiscard]] const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_indexBuffer->GetIndexBufferView(); };
		[[nodiscard]] RHI::ConstantBuffer& GetObjectBuffer() { return m_objectBuffer; };

		[[nodiscard]] uint32_t GetVertexCount() const { return m_vertexCount; }
		[[nodiscard]] uint32_t GetIndexCount() const { return m_indexCount; }

		Material m_material;

	private:
		bool m_needsUpdate;
		RHI::VertexBuffer* m_vertexBuffer;
		RHI::IndexBuffer* m_indexBuffer;
		uint32_t m_vertexCount;
		uint32_t m_indexCount;
		RHI::ConstantBuffer m_objectBuffer;
	};
}
