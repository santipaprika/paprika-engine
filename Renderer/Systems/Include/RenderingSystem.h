#pragma once

#include <stdafx.h>
#include <TransformComponent.h>
#include <CameraComponent.h>
#include <PointLightComponent.h>
#include <InputController.h>
#include <MeshComponent.h> // TODO: Can we move component includes to fwd declaration?
#include <SimpleMath.h>

#include <EntityUtils.h>

using namespace PPK;

class RenderingSystem
{
public:
    RenderingSystem(std::vector<std::optional<TransformComponent>>* transformComponents,
                    std::vector<std::optional<CameraComponent>>* cameraComponents);
    RenderingSystem() = default;

    MeshComponent CreateMeshComponent(MeshComponent::MeshBuildData* inMeshData, const TransformComponent& transform, const Material& material, uint32_t meshIdx, const
                                      std::string& name);
    Entity GetMainCameraId() const;

    void UpdateCameraRenderData(Entity cameraId, uint32_t frameIdx) const;
    uint32_t GetCameraIndexInResourceDescriptorHeap(Entity cameraId, uint32_t frameIdx) const;
    ComPtr<ID3D12Resource> BuildBottomLevelAccelerationStructure(std::span<std::optional<MeshComponent>> meshes);
    RHI::GPUResource* BuildTopLevelAccelerationStructure(ComPtr<ID3D12Resource> BLAS);
    RHI::ConstantBuffer CreateLightsBuffer(std::span<std::optional<PointLightComponent>> pointLights);

    // Pointers to the original component arrays for convenience
    std::vector<std::optional<TransformComponent>>* m_transformComponents;
    std::vector<std::optional<CameraComponent>>* m_cameraComponents;
};