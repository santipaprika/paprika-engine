#include <Entities/MeshEntity.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>
#include <GLTFReader.h>
#include <Timer.h>

std::unique_ptr<PPK::MeshEntity> PPK::MeshEntity::CreateFromGltfMesh(const Microsoft::glTF::Mesh& gltfMesh,
                                                                     const Microsoft::glTF::Document& document)
{
	std::unique_ptr<MeshData> meshData = std::make_unique<MeshData>();

	meshData->m_indices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
		document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);

	if (gltfMesh.primitives[0].HasAttribute("POSITION"))
	{
		meshData->m_vertices = Microsoft::glTF::MeshPrimitiveUtils::GetPositions(
			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
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

	Timer::BeginTimer();
	std::unique_ptr<MeshEntity> meshEntity = std::make_unique<MeshEntity>(std::move(meshData));
	Timer::EndAndReportTimer("Load GLTF attributes");

	return std::move(meshEntity);
}

PPK::MeshEntity::MeshEntity(std::unique_ptr<MeshData> meshData)
{
	m_meshData = std::move(meshData);
}
