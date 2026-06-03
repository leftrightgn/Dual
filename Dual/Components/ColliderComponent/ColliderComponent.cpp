#include "pch.h"
#include "ColliderComponent.h"
#include "Components\SkinnedModelComponent.h"

HEIN::ColliderComponent::ColliderComponent(
	Actor* owner,
	ColliderShape shape
)
	: IComponent(owner)
	, m_shape(shape)
	, m_offset(DirectX::SimpleMath::Vector3::Zero)
	, m_isTrigger(false)
	, m_skinnedModel(nullptr)
	, m_targetBoneName(L"")
	, m_targetBoneNum(-1)
{
}

void HEIN::ColliderComponent::AttachToBone(SkinnedModelComponent* model, const std::wstring& boneName)
{
	m_skinnedModel = model;
	m_targetBoneName = boneName;

	if (m_skinnedModel != nullptr)
	{
		m_targetBoneName = m_skinnedModel->GetBoneIndex(boneName);
	}
}

void HEIN::ColliderComponent::AttachToBone(SkinnedModelComponent* model, const int boneNum)
{
	m_skinnedModel = model;
	m_targetBoneNum = boneNum;
	m_targetBoneName = L"";
}
