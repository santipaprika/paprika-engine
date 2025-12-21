#pragma once

#include <array>
#include <RHI/ConstantBuffer.h>
#include <RHI/Texture.h>

namespace Microsoft::glTF
{
    struct Material;
    class Document;
    struct Mesh;
}

namespace PPK
{
    namespace RHI
    {
        class ShaderDescriptorHeap;
    }

    class MeshComponent;

    enum TextureSlot // Must match GLTF::TextureType
    {
        BaseColor,
        MetallicRoughness,
        Normal,
        Occlusion,
        Emissive,

        COUNT
    };

    struct MaterialRenderResources
    {
        std::array<uint32_t, TextureSlot::COUNT> m_textureIndices;
    };

    class Material
    {
    public:
        Material() = default;
        Material(const Microsoft::glTF::Document& document, const Microsoft::glTF::Material* gltfMaterial);
        [[nodiscard]] std::shared_ptr<RHI::Texture> GetTexture(TextureSlot textureSlot) const;
        void SetTexture(std::shared_ptr<RHI::Texture> texture, TextureSlot textureSlot);
        [[nodiscard]] uint32_t GetIndexInRDH() const;

        void CreateRenderResources();

        // Debug
        std::string GetName() const;
        void SetName(std::string name);
    private:
        std::array<std::shared_ptr<RHI::Texture>, TextureSlot::COUNT> m_pbrTextures;
        std::string m_name;
        std::shared_ptr<RHI::ConstantBuffer> m_renderResourcesBuffer; //< This should be one big buffer with all materials contiguously?
        // shader here
    };
}
