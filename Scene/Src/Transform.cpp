#include <Transform.h>

using namespace DirectX;
namespace PPK
{
	Transform::Transform() : m_objectToWorldMatrix(DirectX::XMMatrixIdentity())
	{
	}

	Transform::Transform(XMMATRIX objectToWorldMatrix) : m_objectToWorldMatrix(objectToWorldMatrix)
	{
	}

	XMFLOAT3 Transform::TransformPoint(XMFLOAT3 p) const
	{
		const XMFLOAT4 hPoint{ p.x, p.y, p.z, 1.f };
		XMFLOAT3 transformedPoint;
		XMStoreFloat3(&transformedPoint, XMVector4Transform(XMLoadFloat4(&hPoint), m_objectToWorldMatrix));
		return transformedPoint;
	}

	XMFLOAT3 Transform::TransformVector(DirectX::XMFLOAT3 v) const
	{
		const XMFLOAT4 hVector{ v.x, v.y, v.z, 0.f };
		XMFLOAT3 transformedPoint;
		XMStoreFloat3(&transformedPoint, XMVector3Transform(XMLoadFloat4(&hVector), m_objectToWorldMatrix));
		return transformedPoint;
	}

	XMMATRIX Transform::GetInverse() const
	{
		XMVECTOR det = XMMatrixDeterminant(m_objectToWorldMatrix);
		return XMMatrixInverse(&det, m_objectToWorldMatrix);
	}
}

