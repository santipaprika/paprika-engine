#pragma once

#include <Transform.h>
#include <Mesh.h>

namespace Microsoft::glTF
{
	struct Mesh;
	class Document;
}

namespace PPK
{
	class MeshEntity
	{
	public:
		static std::unique_ptr<MeshEntity> CreateFromGltfMesh(const Microsoft::glTF::Document& document,
		                                                      const Microsoft::glTF::Mesh& gltfMesh,
		                                                      const Matrix& worldTransform);

		MeshEntity(const MeshEntity&) = delete;
		explicit MeshEntity(std::unique_ptr<Mesh::MeshData> meshData, const Matrix& worldTransform);

		void LoadMeshPrimitives(const Microsoft::glTF::Document& document, const Microsoft::glTF::Mesh& gltfMesh) const;
		void UploadMesh() const;
		void Update();

		// SHOULD BE SMART POINTER! DOES IT GET DELETED?
		Mesh* m_mesh = nullptr;

	private:
		Transform m_transform;
		bool m_dirty;
	};
}
