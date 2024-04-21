#include <Entities/MeshEntity.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>
#include <Timer.h>

#include <DirectXTex.h>
#include <Passes/Pass.h>
#include <RHI/Texture.h>

// From https://github.com/microsoft/glTF-Toolkit/blob/master/glTF-Toolkit/src/GLTFTextureUtils.cpp
DirectX::ScratchImage LoadTexture(const Microsoft::glTF::Document& document, const Microsoft::glTF::Mesh& gltfMesh)
{
	const Microsoft::glTF::Material* material = &document.materials.Get(gltfMesh.primitives[0].materialId);
	const Microsoft::glTF::Texture* texture = &document.textures.Get(material->metallicRoughness.baseColorTexture.textureId);
	const Microsoft::glTF::Image* image = &document.images.Get(texture->imageId);
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

std::unique_ptr<PPK::MeshEntity> PPK::MeshEntity::CreateFromGltfMesh(const Microsoft::glTF::Document& document,
                                                                     const Microsoft::glTF::Mesh& gltfMesh,
                                                                     const Matrix& worldTransform)
{
	std::unique_ptr<Mesh::MeshData> meshData = std::make_unique<Mesh::MeshData>();

	meshData->m_indices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
		document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);

	meshData->m_nIndices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
		document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]).size();

	if (gltfMesh.primitives[0].HasAttribute("POSITION"))
	{
		meshData->m_vertices = Microsoft::glTF::MeshPrimitiveUtils::GetPositions(
			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);

		meshData->m_nVertices = Microsoft::glTF::MeshPrimitiveUtils::GetPositions(
			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]).size() / 3;
	}
	if (gltfMesh.primitives[0].HasAttribute("TEXCOORD_0"))
	{
		meshData->m_uvs = Microsoft::glTF::MeshPrimitiveUtils::GetTexCoords_0(
			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
	}
	if (gltfMesh.primitives[0].HasAttribute("NORMAL"))
	{
		meshData->m_normals = Microsoft::glTF::MeshPrimitiveUtils::GetNormals(
			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
	}
	if (gltfMesh.primitives[0].HasAttribute("COLOR"))
	{
		meshData->m_colors = Microsoft::glTF::MeshPrimitiveUtils::GetColors_0(
			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
	}

	Timer::BeginTimer();
	std::unique_ptr<MeshEntity> meshEntity = std::make_unique<MeshEntity>(std::move(meshData), worldTransform);
	Timer::EndAndReportTimer("Load GLTF attributes");

	// Upload texture
	DirectX::ScratchImage scratchImage = LoadTexture(document, gltfMesh);
	//defaultSampler = RHI::Sampler::CreateSampler();
	// TODO: Handle mips/slices/depth
	meshEntity->m_mesh->m_duckAlbedoTexture = RHI::Texture::CreateTextureResource(scratchImage.GetMetadata(), L"DuckAlbedo", scratchImage.GetImage(0, 0, 0));

	return std::move(meshEntity);
}

PPK::MeshEntity::MeshEntity(std::unique_ptr<Mesh::MeshData> meshData, const Matrix& worldTransform)
{
	m_mesh = Mesh::Create(std::move(meshData));
	m_transform = Transform(worldTransform);
	m_dirty = true;
}

void PPK::MeshEntity::UploadMesh() const
{
	m_mesh->Upload();
}

void PPK::MeshEntity::Update()
{
	if (m_dirty)
	{
		m_mesh->UpdateObjectBuffer(m_transform);
		m_dirty = false;
	}
}
