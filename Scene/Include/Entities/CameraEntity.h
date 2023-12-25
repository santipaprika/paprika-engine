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
		
		explicit CameraEntity(const Camera::CameraDescriptor& cameraGenerationData);

		void MoveCamera(float deltaTime);

		Camera m_camera;

	private:
		//Transform m_normalMatrix;
		Transform m_transform;
		float m_speed;
	};
}
