#include <Transform.h>

using namespace DirectX;

namespace PPK
{
	Transform::Transform() : m_objectToWorldMatrix(Matrix::Identity)
	{
	}

	//Transform::Transform(Matrix objectToWorldMatrix) : m_objectToWorldMatrix(objectToWorldMatrix)
	//{
	//}

	//Transform::Transform(Matrix&& objectToWorldMatrix) : m_objectToWorldMatrix(std::move(objectToWorldMatrix))
	//{
	//}

	Transform::Transform(const Matrix& objectToWorldMatrix) : m_objectToWorldMatrix(objectToWorldMatrix)
	{
	}

	Vector3 Transform::TransformPointToWS(Vector3 p) const
	{
		return Vector3::Transform(p, m_objectToWorldMatrix);
	}

	Vector3 Transform::TransformVectorToWS(Vector3 v) const
	{
		return Vector3::TransformNormal(v, m_objectToWorldMatrix);
	}

	Vector3 Transform::TransformPointToOS(Vector3 p) const
	{
		return Vector3::Transform(p, m_objectToWorldMatrix.Invert());
	}

	Vector3 Transform::TransformVectorToOS(Vector3 v) const
	{
		return Vector3::TransformNormal(v, m_objectToWorldMatrix.Invert());
	}

	Matrix Transform::GetInverse() const
	{
		return m_objectToWorldMatrix.Invert();
	}

	void Transform::SetLocation(const Vector3& newLocation)
	{
		m_objectToWorldMatrix.Translation(newLocation);
	}

	void Transform::Move(const Vector3& positionOffset)
	{
		SetLocation(m_objectToWorldMatrix.Translation() + positionOffset);
	}

	void Transform::Move(float positionOffsetX, float positionOffsetY, float positionOffsetZ)
	{
		Move(Vector3(positionOffsetX, positionOffsetY, positionOffsetZ));
	}
}

