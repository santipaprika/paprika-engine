#pragma once

#include <stdafx_renderer.h>
#include <Camera.h>
#include <Transform.h>
#include <GLTFSDK/GLTF.h>

namespace PPK
{
	// Coordinates system is RH Y-up
	class CameraEntity
	{
	public:
		//struct CameraMatrices
		// NOT WORKING ATM
		static std::unique_ptr<CameraEntity> CreateFromGltfMesh(const Microsoft::glTF::Camera& gltfCamera,
		                                                        const Microsoft::glTF::Document& document);
		
		explicit CameraEntity(Camera::CameraGenerationData&& cameraGenerationData);

	private:
		//Transform m_normalMatrix;
		Transform transform;
		Camera m_camera;
	};
}
