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
		struct CameraInternals
		{
			float m_near = 0.001f;
			float m_far = 100.f;
			float m_fov = 60.f;
			float m_aspectRatio = 1.f;
		};

		struct CameraGenerationData
		{
			CameraInternals m_cameraInternals;
			DirectX::XMFLOAT3 m_position;
			DirectX::XMFLOAT3 m_front;
		};

		//struct CameraMatrices
		// NOT WORKING ATM
		static std::unique_ptr<CameraEntity> CreateFromGltfMesh(const Microsoft::glTF::Camera& gltfCamera,
		                                                        const Microsoft::glTF::Document& document);
		
		explicit CameraEntity(CameraGenerationData cameraGenerationData);

	private:
		Transform m_worldToView;
		Transform m_viewToWorld;
		Transform m_viewToClip;
		//Transform m_normalMatrix;

	};
}
