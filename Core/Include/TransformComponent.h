#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

namespace PPK
{
	// Coordinates system is RH Y-up
	class TransformComponent
	{
	public:
		TransformComponent();
		//Transform(Matrix objectToWorldMatrix);
		explicit TransformComponent(const Matrix& objectToWorldMatrix);
		//
		// TransformComponent(const TransformComponent&) = default;
		// TransformComponent& operator=(const TransformComponent&) = default;
		//
		// TransformComponent(TransformComponent&&) = default;
		// TransformComponent& operator=(TransformComponent&&) = default;
		//
		// // Transform to world space
		// [[nodiscard]] Vector3 TransformPointToWS(Vector3 p) const;
		// [[nodiscard]] Vector3 TransformVectorToWS(Vector3 v) const;
		//
		// // Transform to object space
		// [[nodiscard]] Vector3 TransformPointToOS(Vector3 p) const;
		// [[nodiscard]] Vector3 TransformVectorToOS(Vector3 v) const;
		//
		// [[nodiscard]] Matrix GetInverse() const;
		// void SetLocation(const Vector3& newLocation);
		// void Move(const Vector3& positionOffset);
		// void Move(float positionOffsetX, float positionOffsetY, float positionOffsetZ);
		// void Rotate(const Vector3& rotationOffset);
		// void RotateAndMove(const Vector3& rotationOffset, const Vector3& positionOffset);
		bool m_dirty;

		Matrix m_objectToWorldMatrix;
	private:
	};
}
