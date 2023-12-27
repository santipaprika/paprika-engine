#include <Camera.h>
#include <InputController.h>
#include <Entities/CameraEntity.h>

std::unique_ptr<PPK::CameraEntity> PPK::CameraEntity::CreateFromGltfMesh(const Microsoft::glTF::Camera& gltfCamera,
	const Microsoft::glTF::Document& document)
{
	//Camera::CameraDescriptor cameraDescriptor;

	//Vector3 camPos = Vector3(1, 1, 1);
	//cameraDescriptor.m_transform = Matrix::CreateLookAt(camPos, Vector3::Zero, Vector3::Up /* {0,1,0} */);
	//std::unique_ptr<CameraEntity> meshEntity = std::make_unique<CameraEntity>(std::move(cameraDescriptor));

	return nullptr;
}

PPK::CameraEntity::CameraEntity(const Camera::CameraDescriptor& cameraDescriptor)
	: m_camera(cameraDescriptor), m_transform(cameraDescriptor.m_transform), m_speed(5.f), m_sensibility(100.f)
{
	// CreateBuffer
}

void PPK::CameraEntity::MoveCamera(float deltaTime)
{
	if (!InputController::HasMovementInput() && !InputController::HasMouseInput())
	{
		return;
	}

	Vector3 eulerOffset = Vector3(-InputController::GetMouseOffsetY(), -InputController::GetMouseOffsetX(), 0);
	eulerOffset *= m_sensibility * deltaTime;

	Vector3 positionOffset = Vector3(InputController::IsKeyPressed('D') - InputController::IsKeyPressed('A'),
	                                 InputController::IsKeyPressed('E') - InputController::IsKeyPressed('Q'),
	                                 InputController::IsKeyPressed('S') - InputController::IsKeyPressed('W'));
	positionOffset *= m_speed * deltaTime;
	m_transform.RotateAndMove(eulerOffset, m_transform.TransformVectorToWS(positionOffset));
	//m_transform.Move(m_transform.TransformVectorToWS(positionOffset));

	Camera::CameraDescriptor cameraDescriptor(m_transform);
	m_camera.UpdateCameraMatrices(cameraDescriptor);
}
