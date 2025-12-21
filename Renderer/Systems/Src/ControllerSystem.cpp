#include <stdafx.h>
#include <ControllerSystem.h>
#include <InputController.h>
#include <Timer.h>
#include <TransformComponent.h>
#include <TransformUtils.h>

void ControllerSystem::MoveCamera(CameraComponent& cameraComponent, TransformComponent& transformComponent,
    float deltaTime)
{
    SCOPED_TIMER("ControllerSystem::MoveCamera")
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
        eulerOffset *= cameraComponent.m_sensitivity * 0.01f; //< no need to multiply delta time since cursor position already contains it implicitly
        constexpr float shiftSpeedBoost = 5.f; 
        const float speedFactor = InputController::IsKeyPressed(VK_SHIFT) ? shiftSpeedBoost : 1.f;
        positionOffset *= cameraComponent.m_speed * speedFactor * deltaTime;
        TransformUtils::RotateAndMove(eulerOffset, TransformUtils::TransformVectorToWS(positionOffset, transformComponent.m_renderData.m_objectToWorldMatrix),
                                      transformComponent.m_renderData.m_objectToWorldMatrix);
        transformComponent.m_dirty = false;
        for (int i = 0; i < gFrameCount; i++)
        {
            cameraComponent.m_dirtyRenderState[i] = true;
        }
    }
}