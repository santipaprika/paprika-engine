#include <Renderer.h>
#include <MeshComponent.h>
#include <PassManager.h>
#include <TransformComponent.h>
#include <Passes/BasePass.h>
#include <Passes/DepthPass.h>

namespace PPK
{
    MeshComponent::MeshComponent(const Material& material, RHI::ConstantBuffer&& constantBuffer, RHI::ConstantBuffer&& BLASTransformBuffer,
                                 RHI::VertexBuffer* vertexBuffer, uint32_t vertexCount, RHI::IndexBuffer* indexBuffer,
                                 uint32_t indexCount, const std::string& name)
        : m_material(material), m_objectBuffer(std::move(constantBuffer)), m_BLASTransformBuffer(std::move(BLASTransformBuffer)),
          m_vertexBuffer(vertexBuffer), m_vertexCount(vertexCount), m_indexBuffer(indexBuffer), m_indexCount(indexCount), m_name(name)
    {
    }

    MeshComponent::~MeshComponent()
    {
        // This won't delete anything if it's temporary copy because move constructor sets it to nullptr
        // before leaving the scope
        delete m_vertexBuffer;
        delete m_indexBuffer;
    }

    MeshComponent::MeshComponent(MeshComponent&& other) noexcept
    {
        m_vertexBuffer = other.m_vertexBuffer;
        m_indexBuffer = other.m_indexBuffer;
        
        other.m_vertexBuffer = nullptr;
        other.m_indexBuffer = nullptr;
    
        m_material = std::move(other.m_material);
        m_needsUpdate = other.m_needsUpdate;
        m_vertexCount = other.m_vertexCount;
        m_indexCount = other.m_indexCount;
        m_objectBuffer = std::move(other.m_objectBuffer);
        m_BLASTransformBuffer = std::move(other.m_BLASTransformBuffer);

        m_name = other.m_name;
    }

    void MeshComponent::InitScenePassData()
    {
        BasePassData basePassData;
        basePassData.m_name = m_name.c_str();
        basePassData.m_indexCount = GetIndexCount();
        basePassData.m_vertexBufferView = GetVertexBufferView();
        basePassData.m_indexBufferView = GetIndexBufferView();

        DepthPassData depthPassData;
        depthPassData.m_name = m_name.c_str();
        depthPassData.m_indexCount = GetIndexCount();
        depthPassData.m_vertexBufferView = GetVertexBufferView();
        depthPassData.m_indexBufferView = GetIndexBufferView();

        ShadowVariancePassData shadowVariancePassData;
        shadowVariancePassData.m_name = m_name.c_str();
        shadowVariancePassData.m_indexCount = GetIndexCount();
        shadowVariancePassData.m_vertexBufferView = GetVertexBufferView();
        shadowVariancePassData.m_indexBufferView = GetIndexBufferView();

        // We have two frames in flight with a resource descriptor heap each, but the indices are the same
        // so no need to keep separate copies of them.
        basePassData.m_objectRdhIndex = GetObjectBuffer().GetIndexInRDH(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        depthPassData.m_objectRdhIndex = basePassData.m_objectRdhIndex;
        shadowVariancePassData.m_objectRdhIndex = basePassData.m_objectRdhIndex;

        // Indices in material location (only base color for now)
        basePassData.m_materialRdhIndex = m_material.GetIndexInRDH();

        // Add mesh to be drawn in base pass if albedo is valid
        if (m_material.GetTexture(BaseColor))
        {
            gPassManager->m_depthPass.AddDepthPassRun(depthPassData);
            gPassManager->m_shadowVariancePass.AddShadowVariancePassRun(shadowVariancePassData);
            gPassManager->m_basePass.AddBasePassRun(basePassData);
        }
    }
}
