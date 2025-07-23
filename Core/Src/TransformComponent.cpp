#include <TransformComponent.h>

using namespace DirectX;

namespace PPK
{
	TransformComponent::TransformComponent() : m_dirty(true), m_renderData({
		                                           Matrix::Identity,
		                                           XMFLOAT3X4A(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0)
	                                           })
	{
	}

	TransformComponent::TransformComponent(const Matrix& objectToWorldMatrix, const XMFLOAT3X4A& objectToWorldNormalMatrix) :
		m_dirty(true),
		m_renderData({
			objectToWorldMatrix, objectToWorldNormalMatrix
		})
	{
	}

	//Transform::Transform(Matrix&& objectToWorldMatrix) : m_objectToWorldMatrix(std::move(objectToWorldMatrix))
	//{
	//}

	
}

