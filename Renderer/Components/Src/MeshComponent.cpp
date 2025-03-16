#include <Renderer.h>
#include <MeshComponent.h>
#include <TransformComponent.h>

namespace PPK
{
    MeshComponent::MeshComponent(const Material& material, RHI::ConstantBuffer&& constantBuffer,
                                 RHI::VertexBuffer* vertexBuffer, uint32_t vertexCount, RHI::IndexBuffer* indexBuffer,
                                 uint32_t indexCount)
        : m_material(material), m_objectBuffer(std::move(constantBuffer)), m_vertexBuffer(vertexBuffer),
          m_vertexCount(vertexCount), m_indexBuffer(indexBuffer), m_indexCount(indexCount)
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
    }
}
