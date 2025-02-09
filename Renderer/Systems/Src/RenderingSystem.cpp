#include <Renderer.h>
#include <RenderingSystem.h>
#include <TransformComponent.h>

Vector3 RenderingSystem::TransformPointToWS(Vector3 p, const Matrix& objectToWorldMatrix) const
{
    return Vector3::Transform(p, objectToWorldMatrix);
}

Vector3 RenderingSystem::TransformVectorToWS(Vector3 v, const Matrix& objectToWorldMatrix) const
{
    return Vector3::TransformNormal(v, objectToWorldMatrix);
}

Vector3 RenderingSystem::TransformPointToOS(Vector3 p, const Matrix& objectToWorldMatrix) const
{
    return Vector3::Transform(p, objectToWorldMatrix.Invert());
}

Vector3 RenderingSystem::TransformVectorToOS(Vector3 v, const Matrix& objectToWorldMatrix) const
{
    return Vector3::TransformNormal(v, objectToWorldMatrix.Invert());
}

Matrix RenderingSystem::GetInverseTransform(const Matrix& objectToWorldMatrix) const
{
    return objectToWorldMatrix.Invert();
}

void RenderingSystem::SetTransformLocation(const Vector3& newLocation, Matrix& objectToWorldMatrix)
{
    objectToWorldMatrix.Translation(newLocation);
}

void RenderingSystem::MoveTransform(const Vector3& positionOffset, Matrix& objectToWorldMatrix)
{
    SetTransformLocation(objectToWorldMatrix.Translation() + positionOffset, objectToWorldMatrix);
}

void RenderingSystem::MoveTransform(float positionOffsetX, float positionOffsetY, float positionOffsetZ,
    Matrix& objectToWorldMatrix)
{
    MoveTransform(Vector3(positionOffsetX, positionOffsetY, positionOffsetZ), objectToWorldMatrix);
}

void RenderingSystem::Rotate(const Vector3& rotationOffset, Matrix& objectToWorldMatrix)
{
    const Vector3 previousLocation = objectToWorldMatrix.Translation();
    objectToWorldMatrix = Matrix::CreateFromYawPitchRoll(objectToWorldMatrix.ToEuler() + rotationOffset);
    SetTransformLocation(previousLocation, objectToWorldMatrix);
}

void RenderingSystem::RotateAndMove(const Vector3& rotationOffset, const Vector3& positionOffset,
    Matrix& objectToWorldMatrix)
{
    const Vector3 previousLocation = objectToWorldMatrix.Translation();
    constexpr float pitchLimit = DirectX::XMConvertToRadians(75.f);
    Vector3 finalRotation = rotationOffset + objectToWorldMatrix.ToEuler();
    finalRotation.x = std::min(std::max(finalRotation.x, -pitchLimit), pitchLimit);
    objectToWorldMatrix = Matrix::CreateFromYawPitchRoll(finalRotation);
    SetTransformLocation(previousLocation + positionOffset, objectToWorldMatrix);
}

void RenderingSystem::UpdateConstantBufferData(RHI::ConstantBuffer& constantBuffer,
    const void* data, uint32_t bufferSize)
{
    constantBuffer.SetConstantBufferData(data, bufferSize);
}

void RenderingSystem::UpdateCameraMatrices(const CameraComponent::CameraDescriptor& cameraDescriptor,
    CameraComponent& camera, TransformComponent& transform)
{
    CameraComponent::CameraMatrices cameraMatrices;
    cameraMatrices.m_viewToWorld = transform.m_objectToWorldMatrix;
    cameraMatrices.m_worldToView = GetInverseTransform(cameraMatrices.m_viewToWorld);
    cameraMatrices.m_viewToClip = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
        cameraDescriptor.m_cameraInternals.m_fov, cameraDescriptor.m_cameraInternals.m_aspectRatio,
        cameraDescriptor.m_cameraInternals.m_near,
        cameraDescriptor.m_cameraInternals.m_far);

    UpdateConstantBufferData(camera.GetConstantBuffer(), (void*)&cameraMatrices, sizeof(CameraComponent::CameraMatrices));
}

void RenderingSystem::MoveCamera(CameraComponent& cameraComponent, TransformComponent& transformComponent,
    float deltaTime)
{
    if (!InputController::HasMovementInput() && !InputController::HasMouseInput())
    {
        return;
    }

    Vector3 eulerOffset = Vector3(-InputController::GetMouseOffsetY(), -InputController::GetMouseOffsetX(), 0);
    Vector3 positionOffset = Vector3(InputController::IsKeyPressed('D') - InputController::IsKeyPressed('A'),
                                     InputController::IsKeyPressed('E') - InputController::IsKeyPressed('Q'),
                                     InputController::IsKeyPressed('S') - InputController::IsKeyPressed('W'));
    transformComponent.m_dirty = eulerOffset.LengthSquared() > EPS_FLOAT || positionOffset.LengthSquared() > EPS_FLOAT;

    if (transformComponent.m_dirty)
    {
        eulerOffset *= cameraComponent.m_sensitivity * deltaTime;
        positionOffset *= cameraComponent.m_speed * deltaTime;
        RotateAndMove(eulerOffset, TransformVectorToWS(positionOffset, transformComponent.m_objectToWorldMatrix),
            transformComponent.m_objectToWorldMatrix);
        //m_transform.Move(m_transform.TransformVectorToWS(positionOffset));

        CameraComponent::CameraDescriptor cameraDescriptor;
        // TODO: Should be moved to system
        UpdateCameraMatrices(cameraDescriptor, cameraComponent, transformComponent);

        transformComponent.m_dirty = false;
    }
}
