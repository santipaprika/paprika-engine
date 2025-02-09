#include <TransformComponent.h>

using namespace DirectX;

namespace PPK
{
	TransformComponent::TransformComponent() : m_dirty(true), m_objectToWorldMatrix(Matrix::Identity)
	{
	}

	TransformComponent::TransformComponent(const Matrix& objectToWorldMatrix) : m_dirty(true),
		m_objectToWorldMatrix(objectToWorldMatrix)
	{
	}

	//Transform::Transform(Matrix&& objectToWorldMatrix) : m_objectToWorldMatrix(std::move(objectToWorldMatrix))
	//{
	//}

	
}

