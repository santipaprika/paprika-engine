#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;
namespace TransformUtils
{
    Vector3 TransformPointToWS(Vector3 p, const Matrix& objectToWorldMatrix);
    Vector3 TransformVectorToWS(Vector3 v, const Matrix& objectToWorldMatrix);
    Vector3 TransformPointToOS(Vector3 p, const Matrix& objectToWorldMatrix);
    Vector3 TransformVectorToOS(Vector3 v, const Matrix& objectToWorldMatrix);
    Matrix GetInverseTransform(const Matrix& objectToWorldMatrix);
    void SetTransformLocation(const Vector3& newLocation, Matrix& objectToWorldMatrix);
    void MoveTransform(const Vector3& positionOffset, Matrix& objectToWorldMatrix);
    void MoveTransform(float positionOffsetX, float positionOffsetY, float positionOffsetZ, Matrix& objectToWorldMatrix);
    void Rotate(const Vector3& rotationOffset, Matrix& objectToWorldMatrix);
    void RotateAndMove(const Vector3& rotationOffset, const Vector3& positionOffset, Matrix& objectToWorldMatrix);
};