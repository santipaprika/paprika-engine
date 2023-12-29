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
		static std::unique_ptr<CameraEntity> CreateFromGltfCamera(const Microsoft::glTF::Camera& gltfCamera,
		                                                          const Matrix& globalTransform);
		
		explicit CameraEntity(const Camera::CameraDescriptor& cameraDescriptor);

		void MoveCamera(float deltaTime);

		Camera m_camera;

	private:
		//Transform m_normalMatrix;
		Transform m_transform;
		float m_speed;
		float m_sensibility;
	};
}
