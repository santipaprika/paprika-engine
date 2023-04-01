#pragma once

#include <stdafx_renderer.h>
#include <Transform.h>
#include <GLTFSDK/GLTF.h>

#include <Mesh.h>

namespace PPK
{
	class MeshEntity
	{
	public:
		static std::unique_ptr<MeshEntity> CreateFromGltfMesh(const Microsoft::glTF::Mesh& gltfMesh,
		                                                      const Microsoft::glTF::Document& document);

		MeshEntity(const MeshEntity&) = delete;
		explicit MeshEntity(std::unique_ptr<Mesh::MeshData> meshData);

		void UploadMesh(Renderer &renderer) const;
		Mesh* m_mesh = nullptr;

	private:
		Transform m_transform;
	};
}
