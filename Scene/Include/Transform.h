#pragma once

#include <DirectXMath.h>

namespace PPK
{
	// Coordinates system is RH Y-up
	class Transform
	{
	public:
		Transform();
		Transform(DirectX::XMMATRIX objectToWorldMatrix);

		[[nodiscard]] DirectX::XMFLOAT3 TransformPoint(DirectX::XMFLOAT3 p) const;
		[[nodiscard]] DirectX::XMFLOAT3 TransformVector(DirectX::XMFLOAT3 v) const;

		[[nodiscard]] DirectX::XMMATRIX GetInverse() const;

	private:
		DirectX::XMMATRIX m_objectToWorldMatrix;

	};
}
