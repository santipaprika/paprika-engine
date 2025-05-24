#include <codecvt>
#include <stdafx_renderer.h>
#include <Material.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>
#include <DirectXTex.h>
#include <GLTFReader.h>
#include <locale>

// Based on https://github.com/microsoft/glTF-Toolkit/blob/master/glTF-Toolkit/src/GLTFTextureUtils.cpp
DirectX::ScratchImage LoadTexture(const Microsoft::glTF::Document& document, const Microsoft::glTF::Texture* texture, std::wstring& name)
{
    const Microsoft::glTF::Image* image = &document.images.Get(texture->imageId);
    name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(image->name);
    std::vector<uint8_t> imageData = PPK::GLTFReader::m_gltfResourceReader->ReadBinaryData(document, *image);

    DirectX::TexMetadata info;
    constexpr bool treatAsLinear = true;
    DirectX::ScratchImage scratchImage;
    if (FAILED(DirectX::LoadFromDDSMemory(imageData.data(), imageData.size(), DirectX::DDS_FLAGS_NONE, &info, scratchImage)))
    {
        // DDS failed, try WIC
        // Note: try DDS first since WIC can load some DDS (but not all), so we wouldn't want to get 
        // a partial or invalid DDS loaded from WIC.
        if (FAILED(DirectX::LoadFromWICMemory(imageData.data(), imageData.size(), treatAsLinear ? DirectX::WIC_FLAGS_IGNORE_SRGB : DirectX::WIC_FLAGS_NONE, &info, scratchImage)))
        {
            throw Microsoft::glTF::GLTFException("Failed to load image - Image could not be loaded as DDS or read by WIC.");
        }
    }

    return scratchImage;
    /*if (info.format == DXGI_FORMAT_R32G32B32A32_FLOAT && treatAsLinear)
    {
        return scratchImage;
    }
    else
    {
        DirectX::ScratchImage converted;
        if (FAILED(DirectX::Convert(*scratchImage.GetImage(0, 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, treatAsLinear ? DirectX::TEX_FILTER_DEFAULT : DirectX::TEX_FILTER_SRGB_IN, DirectX::TEX_THRESHOLD_DEFAULT, converted)))
        {
            throw Microsoft::glTF::GLTFException("Failed to convert texture to DXGI_FORMAT_R32G32B32A32_FLOAT for processing.");
        }

        return converted;
    }*/
}

namespace PPK
{
    Material::Material(const Microsoft::glTF::Document& document, const Microsoft::glTF::Material* gltfMaterial)
    {
        // Load and initialize texture resources from GLTF material
        const std::array<std::string, TextureSlot::COUNT> gltfTexturesId = {
            gltfMaterial->metallicRoughness.baseColorTexture.textureId,
            gltfMaterial->metallicRoughness.metallicRoughnessTexture.textureId,
            gltfMaterial->normalTexture.textureId,
            gltfMaterial->occlusionTexture.textureId,
            gltfMaterial->emissiveTexture.textureId
        };

        constexpr std::array<const wchar_t*, TextureSlot::COUNT> slotNames = {
            L"_BaseColor",
            L"_MetallicRoughness",
            L"_Normal",
            L"_Occlusion",
            L"_Emissive"
        };

        for (int slot = 0; slot < TextureSlot::COUNT; slot++)
        {
            // Upload texture
            std::wstring textureName;

            if (!gltfTexturesId[slot].empty())
            {
                const Microsoft::glTF::Texture* gltfTexture = &document.textures.Get(gltfTexturesId[slot]);
                DirectX::ScratchImage scratchImage = LoadTexture(document, gltfTexture, textureName);
                textureName += slotNames[slot];
                //defaultSampler = RHI::Sampler::CreateSampler();
                // TODO: Handle mips/slices/depth
                std::shared_ptr<PPK::RHI::Texture> texture = PPK::RHI::CreateTextureResource(
                    scratchImage.GetMetadata(),
                    textureName.c_str(),
                    scratchImage.GetImage(0, 0, 0)
                );

                SetTexture(texture, static_cast<TextureSlot>(slot));
            }
        }
    }

    std::shared_ptr<RHI::Texture> Material::GetTexture(TextureSlot textureSlot)
    {
        return m_textures[textureSlot];
    }

    void Material::SetTexture(std::shared_ptr<RHI::Texture> texture, TextureSlot textureSlot)
    {
        m_textures[textureSlot] = texture;
    }
}
