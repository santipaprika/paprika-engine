#pragma once

#include <RHI/VertexBuffer.h>

namespace PPK
{
	struct RenderContext;

	class Mesh
	{
	public:
        struct MeshData
        {
			uint32_t m_nVertices;
			uint32_t m_nIndices;

			std::vector<uint32_t> m_indices;
            std::vector<float> m_vertices;
            std::vector<float> m_normals;
            std::vector<float> m_uvs;
            std::vector<uint32_t> m_colors;
        };

        explicit Mesh(std::unique_ptr<MeshData> meshData);

		void Upload(Renderer& renderer);

		static Mesh* Create(std::unique_ptr<MeshData> meshData);
		static Mesh* GetMesh(uint32_t meshId) { return &m_meshes[meshId]; };
		static Mesh* GetLastMesh() { return &m_meshes.back(); };
		static std::vector<Mesh>& GetMeshes() { return m_meshes; };
		// Most passes will iterate over meshes, so it's better to have them in
		// contiguous memory to optimize cache usage
		static std::vector<Mesh> m_meshes;

		struct Vertex
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT2 uv;
			DirectX::XMFLOAT3 normal;
			DirectX::XMFLOAT4 color;
		};

        [[nodiscard]] RHI::VertexBuffer* GetVertexBuffer() const { return m_vertexBuffer.get(); };
        [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_vertexBuffer->GetVertexBufferView(); };
		[[nodiscard]] RHI::IndexBuffer* GetIndexBuffer() const { return m_indexBuffer.get(); };
		[[nodiscard]] D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_indexBuffer->GetIndexBufferView(); };

		[[nodiscard]] uint32_t GetVertexCount() const { return m_vertexCount; }

	private:
        std::unique_ptr<MeshData> m_meshData;
		std::unique_ptr<RHI::VertexBuffer> m_vertexBuffer;
		std::unique_ptr<RHI::IndexBuffer> m_indexBuffer;
		uint32_t m_vertexCount;
	};
}
