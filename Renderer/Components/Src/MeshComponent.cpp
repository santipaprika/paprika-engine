#include <Renderer.h>
#include <MeshComponent.h>
#include <PassManager.h>
#include <TransformComponent.h>
#include <Passes/BasePass.h>

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

        // Copy descriptors to shader visible heap
        for (int frameIdx = 0; frameIdx < gFrameCount; frameIdx++)
        {
            RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
            // Descriptors in object location (only transform for now)
            basePassData.m_objectHandle[frameIdx] = cbvSrvHeap->CopyDescriptors(&GetObjectBuffer(), RHI::HeapLocation::OBJECTS); //< maybe should be method inside component?;
            // Descriptors in material location (only base color for now)
            basePassData.m_materialHandle[frameIdx] = m_material.CopyDescriptors(cbvSrvHeap);
        }

        // Add mesh to be drawn in base pass if albedo is valid
        if (m_material.GetTexture(BaseColor))
        {
            gPassManager->m_basePass.AddBasePassRun(basePassData);
        }
    }
}
