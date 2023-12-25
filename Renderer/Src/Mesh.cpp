#include <Mesh.h>

namespace PPK
{
	Mesh::Mesh(std::unique_ptr<MeshData> meshData)
		: m_meshData(std::move(meshData))
	{
        m_vertexCount = m_meshData->m_nVertices;
        m_indexCount = m_meshData->m_nIndices;
	}

    std::vector<Mesh> Mesh::m_meshes{};

    void Mesh::Upload(Renderer& renderer)
    {
        // Create the vertex buffer.
        constexpr float aspectRatio = 1.f;

		// Define the geometry for a triangle.
        // Vertex triangleVertices[] =
        // {
        //     { { -1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        //     { { 1.f, -1.f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        //     { { -1.f, -1.f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        // };

        const MeshData& meshData = *m_meshData.get();
        std::vector<Vertex> vertexAttributes;
        vertexAttributes.reserve(m_vertexCount);

        const Vector3* groupedPos = reinterpret_cast<const Vector3*>(meshData.m_vertices.data());
        const Vector2* groupedUvs = reinterpret_cast<const Vector2*>(meshData.m_uvs.data());
        const Vector3* groupedNormals = reinterpret_cast<const Vector3*>(meshData.m_normals.data());
        const Vector4* groupedColors = reinterpret_cast<const Vector4*>(meshData.m_colors.data());

        for (int i = 0; i < m_vertexCount; i++)
        {
            vertexAttributes.push_back(
                { groupedPos[i], groupedUvs[i], groupedNormals[i], groupedColors ? groupedColors[i] : Vector4(0,0,0,0) }
            );
        }

        m_vertexBuffer.reset(RHI::VertexBuffer::CreateVertexBuffer(vertexAttributes.data(), sizeof(Vertex), sizeof(Vertex) * m_vertexCount, renderer));
        m_indexBuffer.reset(RHI::IndexBuffer::CreateIndexBuffer(m_meshData->m_indices.data(), sizeof(uint32_t) * m_indexCount, renderer));
    }

    Mesh* Mesh::Create(std::unique_ptr<MeshData> meshData)
    {
        m_meshes.push_back(Mesh(std::move(meshData)));
        return GetLastMesh();
    }
}
