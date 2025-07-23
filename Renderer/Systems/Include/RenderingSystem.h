#pragma once

#include <stdafx.h>
#include <TransformComponent.h>
#include <CameraComponent.h>
#include <InputController.h>
#include <MeshComponent.h> // TODO: Can we move component includes to fwd declaration?
#include <SimpleMath.h>

using namespace PPK;

class RenderingSystem
{
public:
    Vector3 TransformPointToWS(Vector3 p, const Matrix& objectToWorldMatrix) const;
    Vector3 TransformVectorToWS(Vector3 v, const Matrix& objectToWorldMatrix) const;
    Vector3 TransformPointToOS(Vector3 p, const Matrix& objectToWorldMatrix) const;
    Vector3 TransformVectorToOS(Vector3 v, const Matrix& objectToWorldMatrix) const;
    Matrix GetInverseTransform(const Matrix& objectToWorldMatrix) const;
    void SetTransformLocation(const Vector3& newLocation, Matrix& objectToWorldMatrix);
    void MoveTransform(const Vector3& positionOffset, Matrix& objectToWorldMatrix);
    void MoveTransform(float positionOffsetX, float positionOffsetY, float positionOffsetZ, Matrix& objectToWorldMatrix);
    void Rotate(const Vector3& rotationOffset, Matrix& objectToWorldMatrix);
    void RotateAndMove(const Vector3& rotationOffset, const Vector3& positionOffset, Matrix& objectToWorldMatrix);
    void UpdateConstantBufferData(RHI::ConstantBuffer& constantBuffer, const void* data, uint32_t bufferSize);
    void UpdateCameraMatrices(const CameraComponent::CameraDescriptor& cameraDescriptor, CameraComponent& camera, TransformComponent& transform);

#define EPS_FLOAT 1e-16f
    void MoveCamera(CameraComponent& cameraComponent, TransformComponent& transformComponent, float deltaTime);
    MeshComponent CreateMeshComponent(MeshComponent::MeshBuildData* inMeshData, const TransformComponent& transform, const Material& material, uint32_t meshIdx, const
                                      std::string& name);

    ComPtr<ID3D12Resource> BuildBottomLevelAccelerationStructure(std::span<std::optional<MeshComponent>> meshes);
    RHI::GPUResource* BuildTopLevelAccelerationStructure(ComPtr<ID3D12Resource> BLAS);
};