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
    MeshComponent CreateMeshComponent(MeshComponent::MeshBuildData* inMeshData, const TransformComponent& transform, const Material& material, uint32_t meshIdx, const
                                      std::string& name);

    ComPtr<ID3D12Resource> BuildBottomLevelAccelerationStructure(std::span<std::optional<MeshComponent>> meshes);
    RHI::GPUResource* BuildTopLevelAccelerationStructure(ComPtr<ID3D12Resource> BLAS);
};