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
            std::vector<uint32_t> m_indices;
            std::vector<float> m_vertices;
            std::vector<float> m_normals;
            std::vector<float> m_uvs;
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
			DirectX::XMFLOAT4 color;
		};

        [[nodiscard]] RHI::VertexBuffer* GetVertexBuffer() const { return m_vertexBuffer.get(); };
        [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_vertexBuffer->GetVertexBufferView(); };

	private:
        std::unique_ptr<MeshData> m_meshData;
		std::unique_ptr<RHI::VertexBuffer> m_vertexBuffer;
	};
}
