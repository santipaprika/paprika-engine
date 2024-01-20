#include <Entities/MeshEntity.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>
#include <Timer.h>

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

	return std::move(meshEntity);
}

PPK::MeshEntity::MeshEntity(std::unique_ptr<Mesh::MeshData> meshData, const Matrix& worldTransform)
{
	m_mesh = Mesh::Create(std::move(meshData));
	m_transform = Transform(worldTransform);
}

void PPK::MeshEntity::UploadMesh() const
{
	m_mesh->Upload();
}
