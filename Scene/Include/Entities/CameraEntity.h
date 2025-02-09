// #pragma once
//
// #include <CameraComponent.h>
// #include <TransformComponent.h>
//
// namespace Microsoft::glTF
// {
// 	struct Camera;
// }
//
// namespace PPK
// {
// 	// Coordinates system is RH Y-up
// 	class CameraEntity
// 	{
// 	public:
// 		static std::unique_ptr<CameraEntity> CreateFromGltfCamera(const Microsoft::glTF::Camera& gltfCamera,
// 		                                                          const Matrix& globalTransform);
// 		
// 		explicit CameraEntity(const CameraComponent::CameraDescriptor& cameraDescriptor);
//
// 		void MoveCamera(float deltaTime);
//
// 		// CameraComponent m_camera;
//
// 	private:
// 		//Transform m_normalMatrix;
// 		TransformComponent m_transform;
//
// 	};
// }
