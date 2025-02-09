// #pragma once
//
// #include <TransformComponent.h>
// #include <MeshComponent.h>
//
// namespace Microsoft::glTF
// {
// 	struct Mesh;
// 	class Document;
// }
//
// namespace PPK
// {
// 	class MeshEntity
// 	{
// 	public:
// 		static MeshEntity* CreateFromGltfMesh(const Microsoft::glTF::Document& document,
// 		                                                      const Microsoft::glTF::Mesh& gltfMesh,
// 		                                                      const Matrix& worldTransform);
//
// 		MeshEntity(const MeshEntity&) = delete;
// 		explicit MeshEntity(MeshComponent::MeshBuildData* meshData, const Matrix& worldTransform);
//
// 		void LoadMeshPrimitives(const Microsoft::glTF::Document& document, const Microsoft::glTF::Mesh& gltfMesh) const;
// 		void UploadMesh() const;
// 		void Update();
//
// 		// SHOULD BE SMART POINTER! DOES IT GET DELETED?
// 		MeshComponent* m_mesh = nullptr;
//
// 	private:
// 		TransformComponent m_transform;
// 		bool m_dirty;
// 	};
// }
