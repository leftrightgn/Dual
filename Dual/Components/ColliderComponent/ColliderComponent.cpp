#include "pch.h"
#include "ColliderComponent.h"
#include "Components/TransformComponent.h"
#include "Entities/Actor.h"

HEIN::ColliderComponent::ColliderComponent(
	Actor* owner,
	ColliderShape shape
)
	: IComponent(owner)
	, m_shape(shape)
	, m_offset(DirectX::SimpleMath::Vector3::Zero)
	, m_rotationOffset(DirectX::SimpleMath::Quaternion::Identity)
	, m_isTrigger(false)
{
}

void HEIN::ColliderComponent::Start()
{
	if (GetOwner() != nullptr)
	{
		m_transform = GetOwner()->GetComponent<TransformComponent>();
	}
}


DirectX::SimpleMath::Matrix HEIN::ColliderComponent::CalculateWorldMatrix()
{
	DirectX::SimpleMath::Matrix localOffset =
		DirectX::SimpleMath::Matrix::CreateFromQuaternion(m_rotationOffset) * 
		DirectX::SimpleMath::Matrix::CreateTranslation(m_offset);

	if (m_useManualMatrix)
	{
		return localOffset * m_manualMatrix;
	}

	DirectX::SimpleMath::Matrix finalMatrix = localOffset;

	if (m_transform != nullptr)
	{
		finalMatrix = finalMatrix * m_transform->GetWorldMatrix();
	}

	return finalMatrix;
}
