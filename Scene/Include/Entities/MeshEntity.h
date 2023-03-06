#pragma once

#include <GLTFReader.h>
#include <stdafx_renderer.h>
#include <Transform.h>
#include <GLTFSDK/GLTF.h>

namespace PPK
{
	class MeshEntity
	{
	public:
		struct MeshData
		{
			std::vector<uint32_t> m_indices;
			std::vector<float> m_vertices;
			std::vector<float> m_normals;
			std::vector<float> m_uvs;
		};

		static std::unique_ptr<MeshEntity> CreateFromGltfMesh(const Microsoft::glTF::Mesh& gltfMesh,
		                                                      const Microsoft::glTF::Document& document);

		MeshEntity(const MeshEntity&) = delete;
		explicit MeshEntity(std::unique_ptr<MeshData> meshData);

	private:
		std::unique_ptr<MeshData> m_meshData{};
		Transform m_transform;
	};
}
