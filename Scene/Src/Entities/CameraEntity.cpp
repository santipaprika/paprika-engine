#include <Entities/CameraEntity.h>

std::unique_ptr<PPK::CameraEntity> PPK::CameraEntity::CreateFromGltfMesh(const Microsoft::glTF::Camera& gltfCamera,
	const Microsoft::glTF::Document& document)
{
	
}

PPK::CameraEntity::CameraEntity(CameraGenerationData cameraGenerationData)
{
	auto camPos = DirectX::XMLoadFloat3(&cameraGenerationData.m_position);
	auto camFrontDir = DirectX::XMLoadFloat3(&cameraGenerationData.m_front);
	m_viewToWorld = DirectX::XMMatrixLookToRH(camPos, camFrontDir, DirectX::FXMVECTOR{0.f, 1.f, 0.f});
	m_worldToView = m_viewToWorld.GetInverse();

	m_viewToClip = DirectX::XMMatrixPerspectiveFovRH(cameraGenerationData.m_cameraInternals.m_fov, 1.f,
	                                                 cameraGenerationData.m_cameraInternals.m_near,
	                                                 cameraGenerationData.m_cameraInternals.m_far);
}
