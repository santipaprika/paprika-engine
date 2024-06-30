#pragma once

#include <RHI/VertexBuffer.h>

#include <SimpleMath.h>
#include <RHI/ConstantBuffer.h>
#include <RHI/Texture.h> // For hardcoded albedo, remove

using namespace DirectX::SimpleMath;

namespace PPK
{
	struct RenderContext;
	class Transform;

	// Low level interpretation of camera which interacts with RHI and sets up the required buffers
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

		struct ObjectData
		{
			Matrix transform;
		};

        explicit Mesh(std::unique_ptr<MeshData> meshData);

		void Upload();

		static Mesh* Create(std::unique_ptr<MeshData> meshData);
		static Mesh* GetMesh(uint32_t meshId) { return &m_meshes[meshId]; };
		static Mesh* GetLastMesh() { return &m_meshes.back(); };
		static std::vector<Mesh>& GetMeshes() { return m_meshes; };
		// Most passes will iterate over meshes, so it's better to have them in
		// contiguous memory to optimize cache usage
		static std::vector<Mesh> m_meshes;

		struct Vertex
		{
			Vector3 position;
			Vector2 uv;
			Vector3 normal;
			Vector4 color;
		};

        [[nodiscard]] RHI::VertexBuffer* GetVertexBuffer() const { return m_vertexBuffer.get(); };
        [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_vertexBuffer->GetVertexBufferView(); };
		[[nodiscard]] RHI::IndexBuffer* GetIndexBuffer() const { return m_indexBuffer.get(); };
		[[nodiscard]] D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_indexBuffer->GetIndexBufferView(); };
		[[nodiscard]] std::shared_ptr<RHI::ConstantBuffer> GetObjectBuffer() const { return m_objectBuffer; };

		[[nodiscard]] uint32_t GetVertexCount() const { return m_vertexCount; }
		[[nodiscard]] uint32_t GetIndexCount() const { return m_indexCount; }

		void CreateObjectConstantBuffer();
		void UpdateObjectBuffer(Transform& transform);
		// TODO: Temp to avoid crash on exit scope, but there's leak on close. Figure out where to store this.
		std::shared_ptr<PPK::RHI::Texture> m_duckAlbedoTexture;

	private:
        std::unique_ptr<MeshData> m_meshData;
		std::unique_ptr<RHI::VertexBuffer> m_vertexBuffer;
		std::unique_ptr<RHI::IndexBuffer> m_indexBuffer;
		uint32_t m_vertexCount;
		uint32_t m_indexCount;
		std::shared_ptr<RHI::ConstantBuffer> m_objectBuffer;
	};
}
