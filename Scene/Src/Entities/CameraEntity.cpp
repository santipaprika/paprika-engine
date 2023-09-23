#include <Camera.h>
#include <Entities/CameraEntity.h>

std::unique_ptr<PPK::CameraEntity> PPK::CameraEntity::CreateFromGltfMesh(const Microsoft::glTF::Camera& gltfCamera,
	const Microsoft::glTF::Document& document)
{
	Camera::CameraGenerationData cameraGenerationData;
	cameraGenerationData.m_position = DirectX::XMFLOAT3(1,1,1);
	std::unique_ptr<CameraEntity> meshEntity = std::make_unique<CameraEntity>(std::move(cameraGenerationData));

	return std::move(meshEntity);
}

PPK::CameraEntity::CameraEntity(Camera::CameraGenerationData&& cameraGenerationData)
	: m_camera(std::move(cameraGenerationData))
{
	// CreateBuffer
}