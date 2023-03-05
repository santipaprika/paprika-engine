#pragma once

#include <GLTFReader.h>
#include <stdafx_renderer.h>
#include <GLTFSDK/GLTF.h>

namespace PPK
{
	class MeshEntity
	{
	public:
		MeshEntity() = default;
		static std::unique_ptr<MeshEntity> CreateFromGltfMesh(const Microsoft::glTF::Mesh& gltfMesh,
		                                                      const Microsoft::glTF::Document& document);

	private:
		std::vector<DirectX::XMINT3> m_indices;
		std::vector<DirectX::XMFLOAT3> m_positions;
	};
}
