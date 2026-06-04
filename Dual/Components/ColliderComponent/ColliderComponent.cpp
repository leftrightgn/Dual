#include "pch.h"
#include "ColliderComponent.h"
#include "Components/TransformComponent.h"
#include "Components/SkinnedModelComponent.h"
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
	, m_skinnedModel(nullptr)
	, m_targetBoneName(L"")
	, m_targetBoneNum(-1)
{
}

void HEIN::ColliderComponent::Start()
{
	if (GetOwner() != nullptr)
	{
		m_transform = GetOwner()->GetComponent<TransformComponent>();
	}
}

void HEIN::ColliderComponent::AttachToBone(SkinnedModelComponent* model, const std::wstring& boneName)
{
	m_skinnedModel = model;
	m_targetBoneName = boneName;

	if (m_skinnedModel != nullptr)
	{
		m_targetBoneNum = m_skinnedModel->GetBoneIndex(boneName);
	}
}

void HEIN::ColliderComponent::AttachToBone(SkinnedModelComponent* model, const int boneNum)
{
	m_skinnedModel = model;
	m_targetBoneNum = boneNum;
	m_targetBoneName = L"";
}

DirectX::SimpleMath::Matrix HEIN::ColliderComponent::CalculateWorldMatrix()
{
	DirectX::SimpleMath::Matrix localOffset =
		DirectX::SimpleMath::Matrix::CreateFromQuaternion(m_rotationOffset) * 
		DirectX::SimpleMath::Matrix::CreateTranslation(m_offset);

	DirectX::SimpleMath::Matrix finalMatrix = localOffset;

	if (m_skinnedModel != nullptr && m_targetBoneNum != -1)
	{
		DirectX::SimpleMath::Matrix actorWorld = m_transform->GetWorldMatrix();
		DirectX::SimpleMath::Matrix boneWorld = m_skinnedModel->GetBoneWorldMatrix(m_targetBoneNum, actorWorld);

		finalMatrix = finalMatrix * boneWorld;
	}

	else if (m_transform != nullptr)
	{
		finalMatrix = finalMatrix * m_transform->GetWorldMatrix();
	}

	return finalMatrix;
}
