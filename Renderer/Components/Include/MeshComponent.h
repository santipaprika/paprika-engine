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

        explicit MeshComponent(MeshBuildData* meshData, const Material& material, uint32_t meshIdx);
		~MeshComponent();
        MeshComponent(MeshComponent&& other) noexcept;

		void Upload();

		// static MeshComponent* Create(MeshBuildData* meshData);
		// static MeshComponent* GetMesh(uint32_t meshId) { return &m_meshes[meshId]; };
		// static MeshComponent* GetLastMesh() { return &m_meshes.back(); };
		// static std::vector<MeshComponent>& GetMeshes() { return m_meshes; };
		// Most passes will iterate over meshes, so it's better to have them in
		// contiguous memory to optimize cache usage
		// static std::vector<MeshComponent> m_meshes;

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

		void CreateObjectConstantBuffer();
		// void UpdateObjectBuffer(TransformComponent& transform);
		// MaterialComponent* m_material;
		Material m_material;

	private:
		bool m_needsUpdate;
        MeshBuildData* m_meshData;
		RHI::VertexBuffer* m_vertexBuffer;
		RHI::IndexBuffer* m_indexBuffer;
		uint32_t m_vertexCount;
		uint32_t m_indexCount;
		RHI::ConstantBuffer m_objectBuffer;
	};
}
