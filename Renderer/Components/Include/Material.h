#pragma once

#include <array>
#include <optional>
#include <RHI/Texture.h>

namespace Microsoft::glTF
{
    struct Material;
    class Document;
    struct Mesh;
}

namespace PPK
{
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
    
    class Material
    {
    public:
        Material() = default;
        Material(const Microsoft::glTF::Document& document, const Microsoft::glTF::Material* gltfMaterial);
        void FillMaterial(const Microsoft::glTF::Document& document, const Microsoft::glTF::Material* gltfMaterial);
        std::shared_ptr<RHI::Texture> GetTexture(TextureSlot textureSlot);
        void SetTexture(std::shared_ptr<RHI::Texture> texture, TextureSlot textureSlot);
    private:
        std::array<std::shared_ptr<RHI::Texture>, TextureSlot::COUNT> m_textures;
        // shader here
    };
}
