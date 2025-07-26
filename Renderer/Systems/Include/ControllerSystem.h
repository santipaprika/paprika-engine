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
    void MoveCamera(CameraComponent& cameraComponent, TransformComponent& transformComponent, float deltaTime);
};