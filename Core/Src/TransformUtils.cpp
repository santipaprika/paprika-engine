#include <stdafx.h>
#include <TransformUtils.h>

Vector3 TransformUtils::TransformPointToWS(Vector3 p, const Matrix& objectToWorldMatrix)
{
    return Vector3::Transform(p, objectToWorldMatrix);
}

Vector3 TransformUtils::TransformVectorToWS(Vector3 v, const Matrix& objectToWorldMatrix)
{
    return Vector3::TransformNormal(v, objectToWorldMatrix);
}

Vector3 TransformUtils::TransformPointToOS(Vector3 p, const Matrix& objectToWorldMatrix)
{
    return Vector3::Transform(p, objectToWorldMatrix.Invert());
}

Vector3 TransformUtils::TransformVectorToOS(Vector3 v, const Matrix& objectToWorldMatrix)
{
    return Vector3::TransformNormal(v, objectToWorldMatrix.Invert());
}

Matrix TransformUtils::GetInverseTransform(const Matrix& objectToWorldMatrix)
{
    return objectToWorldMatrix.Invert();
}

void TransformUtils::SetTransformLocation(const Vector3& newLocation, Matrix& objectToWorldMatrix)
{
    objectToWorldMatrix.Translation(newLocation);
}

void TransformUtils::MoveTransform(const Vector3& positionOffset, Matrix& objectToWorldMatrix)
{
    SetTransformLocation(objectToWorldMatrix.Translation() + positionOffset, objectToWorldMatrix);
}

void TransformUtils::MoveTransform(float positionOffsetX, float positionOffsetY, float positionOffsetZ,
    Matrix& objectToWorldMatrix)
{
    MoveTransform(Vector3(positionOffsetX, positionOffsetY, positionOffsetZ), objectToWorldMatrix);
}

void TransformUtils::Rotate(const Vector3& rotationOffset, Matrix& objectToWorldMatrix)
{
    const Vector3 previousLocation = objectToWorldMatrix.Translation();
    objectToWorldMatrix = Matrix::CreateFromYawPitchRoll(objectToWorldMatrix.ToEuler() + rotationOffset);
    SetTransformLocation(previousLocation, objectToWorldMatrix);
}

void TransformUtils::RotateAndMove(const Vector3& rotationOffset, const Vector3& positionOffset,
    Matrix& objectToWorldMatrix)
{
    const Vector3 previousLocation = objectToWorldMatrix.Translation();
    constexpr float pitchLimit = DirectX::XMConvertToRadians(75.f);
    Vector3 finalRotation = rotationOffset + objectToWorldMatrix.ToEuler();
    finalRotation.x = std::min(std::max(finalRotation.x, -pitchLimit), pitchLimit);
    objectToWorldMatrix = Matrix::CreateFromYawPitchRoll(finalRotation);
    SetTransformLocation(previousLocation + positionOffset, objectToWorldMatrix);
    PPK::Logger::Verbose((std::string("Camera Pos: X: ") + std::to_string(previousLocation.x) + std::string(" Y: ") + std::to_string(previousLocation.y) + std::string(" Z: ") + std::to_string(previousLocation.z)).c_str());
}