#include <Entities/MeshEntity.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>
#include <GLTFReader.h>

std::unique_ptr<PPK::MeshEntity> PPK::MeshEntity::CreateFromGltfMesh(const Microsoft::glTF::Mesh& gltfMesh,
                                                                     const Microsoft::glTF::Document& document)
{
	std::vector<uint32_t> indices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
		document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);

	return nullptr;
}
