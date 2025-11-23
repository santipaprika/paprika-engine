#include <codecvt>
#include <stdafx_renderer.h>
#include <Material.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>
#include <DirectXTex.h>
#include <GLTFReader.h>
#include <locale>
#include <Renderer.h>
#include <RHI/ConstantBuffer.h>
#include <RHI/ShaderDescriptorHeap.h>

// Based on https://github.com/microsoft/glTF-Toolkit/blob/master/glTF-Toolkit/src/GLTFTextureUtils.cpp
DirectX::ScratchImage LoadTexture(const Microsoft::glTF::Document& document, const Microsoft::glTF::Texture* texture, std::string& name)
{
    const Microsoft::glTF::Image* image = &document.images.Get(texture->imageId);
    name = image->name;
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
        SetName(gltfMaterial->name);

        // Load and initialize texture resources from GLTF material
        const std::array<std::string, TextureSlot::COUNT> gltfTexturesId = {
            gltfMaterial->metallicRoughness.baseColorTexture.textureId,
            gltfMaterial->metallicRoughness.metallicRoughnessTexture.textureId,
            gltfMaterial->normalTexture.textureId,
            gltfMaterial->occlusionTexture.textureId,
            gltfMaterial->emissiveTexture.textureId
        };

        constexpr std::array<const char*, TextureSlot::COUNT> slotNames = {
            "_BaseColor",
            "_MetallicRoughness",
            "_Normal",
            "_Occlusion",
            "_Emissive"
        };

        MaterialRenderResources materialRenderResources;
        for (int slot = 0; slot < TextureSlot::COUNT; slot++)
        {
            // Upload texture
            std::string textureName;

            materialRenderResources.m_textureIndices[slot] = INVALID_INDEX;
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

        CreateRenderResources();
    }

    std::shared_ptr<RHI::Texture> Material::GetTexture(TextureSlot textureSlot) const
    {
        return m_pbrTextures[textureSlot];
    }

    void Material::SetTexture(std::shared_ptr<RHI::Texture> texture, TextureSlot textureSlot)
    {
        m_pbrTextures[textureSlot] = texture;
    }

    uint32_t Material::GetIndexInRDH() const
    {
        return m_renderResourcesBuffer->GetIndexInRDH(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    void Material::CreateRenderResources()
    {
        if (m_renderResourcesBuffer) // Make sure we don't create twice
        {
            return;
        }

        MaterialRenderResources materialRenderResources;
        for (int i = 0; i < TextureSlot::COUNT; i++)
        {
            std::shared_ptr<RHI::Texture> texture = m_pbrTextures[i];
            if (texture)
            {
                materialRenderResources.m_textureIndices[i] = texture->GetIndexInRDH(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
        }

        m_renderResourcesBuffer = std::make_shared<RHI::ConstantBuffer>(RHI::ConstantBufferUtils::CreateConstantBuffer(
           sizeof(MaterialRenderResources), ("M_" + m_name).c_str(), &materialRenderResources));
    }

    std::string Material::GetName() const
    {
        return m_name;
    }

    void Material::SetName(std::string name)
    {
        m_name = name;
    }
}
