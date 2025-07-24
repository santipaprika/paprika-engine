#pragma once

#include <CameraComponent.h>

namespace PPK
{
    class TransformComponent;
}

using namespace PPK;

class ControllerSystem
{
public:

    void UpdateCameraMatrices(const CameraComponent::CameraDescriptor& cameraDescriptor, CameraComponent& camera, TransformComponent& transform);
    void MoveCamera(CameraComponent& cameraComponent, TransformComponent& transformComponent, float deltaTime);
};