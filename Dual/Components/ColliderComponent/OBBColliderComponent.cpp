#include "OBBColliderComponent.h"


HEIN::OBBColliderComponent::OBBColliderComponent(Actor* owner)
	: ColliderComponent(owner, ColliderShape::OBB)
	, m_extents(DirectX::SimpleMath::Vector3::Zero)
{
}

void HEIN::OBBColliderComponent::Initialize(const DirectX::SimpleMath::Vector3 extents)
{
	m_extents = extents;
}

void HEIN::OBBColliderComponent::Update(float deltaTime)
{
}

void HEIN::OBBColliderComponent::DrawDebug(GameContext& gameContext)
{
	if (gameContext.m_debugCollisionRenderer == nullptr) return;

	
}
