// #include <stdafx.h>
// #include <Entities/MeshEntity.h>
// #include <GLTFSDK/MeshPrimitiveUtils.h>
// #include <GLTFReader.h>
// #include <Timer.h>
//
// PPK::MeshEntity* PPK::MeshEntity::CreateFromGltfMesh(const Microsoft::glTF::Document& document,
//                                                                      const Microsoft::glTF::Mesh& gltfMesh,
//                                                                      const Matrix& worldTransform)
// {
// 	MeshComponent::MeshBuildData* meshData = new MeshComponent::MeshBuildData();
//
// 	meshData->m_indices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
// 		document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
//
// 	meshData->m_nIndices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
// 		document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]).size();
//
// 	if (gltfMesh.primitives[0].HasAttribute("POSITION"))
// 	{
// 		meshData->m_vertices = Microsoft::glTF::MeshPrimitiveUtils::GetPositions(
// 			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
//
// 		meshData->m_nVertices = Microsoft::glTF::MeshPrimitiveUtils::GetPositions(
// 			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]).size() / 3;
// 	}
// 	if (gltfMesh.primitives[0].HasAttribute("TEXCOORD_0"))
// 	{
// 		meshData->m_uvs = Microsoft::glTF::MeshPrimitiveUtils::GetTexCoords_0(
// 			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
// 	}
// 	if (gltfMesh.primitives[0].HasAttribute("NORMAL"))
// 	{
// 		meshData->m_normals = Microsoft::glTF::MeshPrimitiveUtils::GetNormals(
// 			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
// 	}
// 	if (gltfMesh.primitives[0].HasAttribute("COLOR"))
// 	{
// 		meshData->m_colors = Microsoft::glTF::MeshPrimitiveUtils::GetColors_0(
// 			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
// 	}
//
// 	Timer::BeginTimer();
// 	MeshEntity* meshEntity = new MeshEntity(meshData, worldTransform);
// 	Timer::EndAndReportTimer("Load GLTF attributes");
//
// 	meshEntity->LoadMeshPrimitives(document, gltfMesh);
//
// 	return meshEntity;
// }
//
// PPK::MeshEntity::MeshEntity(MeshComponent::MeshBuildData* meshData, const Matrix& worldTransform)
// {
// 	m_mesh = MeshComponent::Create(meshData);
// 	m_transform = TransformComponent(worldTransform);
// 	m_dirty = true;
// }
//
// void PPK::MeshEntity::LoadMeshPrimitives(const Microsoft::glTF::Document& document, const Microsoft::glTF::Mesh& gltfMesh) const
// {
// 	// Hardcoded to singl primitive per mesh. TODO: Add support for segments
// 	// for gltfMesh.primitives ... etc
// 	const Microsoft::glTF::Material* gltfMaterial = &document.materials.Get(gltfMesh.primitives[0].materialId);
// 	m_mesh->m_material->FillMaterial(document, gltfMaterial);
// }
//
// void PPK::MeshEntity::UploadMesh() const
// {
// 	m_mesh->Upload();
// }
//
// void PPK::MeshEntity::Update()
// {
// 	if (m_dirty)
// 	{
// 		m_mesh->UpdateObjectBuffer(m_transform);
// 		m_dirty = false;
// 	}
// }
