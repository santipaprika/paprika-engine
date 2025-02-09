#include <Renderer.h>
#include <MeshComponent.h>
#include <TransformComponent.h>

namespace PPK
{
    MeshComponent::MeshComponent(MeshBuildData* meshData, const Material& material, uint32_t meshIdx)
        : m_meshData(meshData), m_material(material),
          m_objectBuffer(std::move(*RHI::ConstantBuffer::CreateConstantBuffer(sizeof(ObjectData),
              std::wstring(L"ObjectCB_" + std::to_wstring(meshIdx)).c_str(), true)))
        // TODO: Move to RENDERING SYSTEM and do in batch for all meshes!
    {
        m_vertexCount = m_meshData->m_nVertices;
        m_indexCount = m_meshData->m_nIndices;

        // m_material = new MaterialComponent();

        // CreateObjectConstantBuffer();

        Upload();
    }

    // std::vector<MeshComponent> MeshComponent::m_meshes{};

    void MeshComponent::Upload()
    {
        // Define the geometry for a triangle.
        // Vertex triangleVertices[] =
        // {
        //     { { -1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        //     { { 1.f, -1.f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        //     { { -1.f, -1.f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        // };

        const MeshBuildData& meshData = *m_meshData;
        std::vector<Vertex> vertexAttributes;
        vertexAttributes.reserve(m_vertexCount);

        const Vector3* groupedPos = reinterpret_cast<const Vector3*>(meshData.m_vertices.data());
        const Vector2* groupedUvs = reinterpret_cast<const Vector2*>(meshData.m_uvs.data());
        const Vector3* groupedNormals = reinterpret_cast<const Vector3*>(meshData.m_normals.data());
        const Vector4* groupedColors = reinterpret_cast<const Vector4*>(meshData.m_colors.data());

        for (int i = 0; i < m_vertexCount; i++)
        {
            vertexAttributes.push_back(
                {
                    groupedPos[i], groupedUvs[i], groupedNormals[i],
                    groupedColors ? groupedColors[i] : Vector4(0, 0, 0, 0)
                }
            );
        }

        m_vertexBuffer = RHI::VertexBuffer::CreateVertexBuffer(vertexAttributes.data(), sizeof(Vertex),
                                                               sizeof(Vertex) * m_vertexCount);
        m_indexBuffer = RHI::IndexBuffer::CreateIndexBuffer(m_meshData->m_indices.data(),
                                                            sizeof(uint32_t) * m_indexCount);
    }

    // MeshComponent* MeshComponent::Create(MeshBuildData* meshData)
    // {
    //     m_meshes.push_back(MeshComponent(meshData));
    //     return GetLastMesh();
    // }

    void MeshComponent::CreateObjectConstantBuffer()
    {
        static uint32_t bufferIdx = 0;
        const std::wstring bufferName = L"ObjectCB_" + std::to_wstring(bufferIdx++);

        // m_objectBuffer = std::move(*RHI::ConstantBuffer::CreateConstantBuffer(sizeof(ObjectData), bufferName.c_str(), true));
    }

    // TODO: Use function in RenderSystem
    // void MeshComponent::UpdateObjectBuffer(TransformComponent& transform)
    // {
    //     // Right now ObjectData == Transform
    //     m_objectBuffer.SetConstantBufferData((void*)&transform, sizeof(ObjectData));
    // }
}
