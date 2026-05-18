#include "pch.h"
#include "CharacterMovementComponent.h"
#include "Components/TransformComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Entities/Actor.h"


HEIN::CharacterMovementComponent::CharacterMovementComponent(Actor* owner)
	: IComponent(owner)
	, m_blackboard()
	, m_transform()
	, m_acceleration(DEFAULT_ACCELERATION)
	, m_friction(DEFAULT_FRICTION)
{
}

void HEIN::CharacterMovementComponent::Start()
{
	m_blackboard = m_owner->GetComponent<HEIN::CombatBlackBoard>();
	m_transform = m_owner->GetComponent<HEIN::TransformComponent>();
}

void HEIN::CharacterMovementComponent::Update(float deltaTime)
{
	if (!m_transform || !m_blackboard) return;

	if (m_blackboard->currentStance == HEIN::CombatStance::AttackRelese ||
		m_blackboard->currentStance == HEIN::CombatStance::Staggered)
	{
		m_blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
	}

	// Apply physics momentum directly 
	DirectX::SimpleMath::Vector3 desiredVelocity = m_blackboard->moveIntent * m_acceleration;

	m_blackboard->currentVelocity = DirectX::SimpleMath::Vector3::Lerp(
		m_blackboard->currentVelocity,
		desiredVelocity,
		deltaTime * m_friction
	);

	// Move the actor
	DirectX::SimpleMath::Vector3 pos = m_transform->GetPosition();
	pos += m_blackboard->currentVelocity * deltaTime;
	m_transform->SetPosition(pos);
}
