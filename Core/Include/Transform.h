#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

namespace PPK
{
	// Coordinates system is RH Y-up
	class Transform
	{
	public:
		Transform();
		//Transform(Matrix objectToWorldMatrix);
		Transform(const Matrix& objectToWorldMatrix);

		Transform(const Transform&) = default;
		Transform& operator=(const Transform&) = default;

		Transform(Transform&&) = default;
		Transform& operator=(Transform&&) = default;

		// Transform to world space
		[[nodiscard]] Vector3 TransformPointToWS(Vector3 p) const;
		[[nodiscard]] Vector3 TransformVectorToWS(Vector3 v) const;

		// Transform to object space
		[[nodiscard]] Vector3 TransformPointToOS(Vector3 p) const;
		[[nodiscard]] Vector3 TransformVectorToOS(Vector3 v) const;

		[[nodiscard]] Matrix GetInverse() const;
		void SetLocation(const Vector3& newLocation);
		void Move(const Vector3& positionOffset);
		void Move(float positionOffsetX, float positionOffsetY, float positionOffsetZ);

	private:
		Matrix m_objectToWorldMatrix;
	};
}
