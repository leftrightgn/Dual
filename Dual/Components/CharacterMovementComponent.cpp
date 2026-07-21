#include "pch.h"
#include "CharacterMovementComponent.h"
#include "../../External/Engine/Components/TransformComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Components/RigidBodyComponent.h"


HEIN::CharacterMovementComponent::CharacterMovementComponent(Actor* owner)
	: IComponent(owner)
	, m_blackboard()
	, m_transform()
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

	// Dynamic Speed
	DirectX::SimpleMath::Vector3 desiredVelocity = m_blackboard->moveIntent * m_blackboard->currentSpeed;

	m_blackboard->currentVelocity = DirectX::SimpleMath::Vector3::Lerp(
		m_blackboard->currentVelocity,
		desiredVelocity,
		deltaTime * m_friction
	);

	// Move the actor
	HEIN::RigidBodyComponent* rb = m_owner->GetComponent<HEIN::RigidBodyComponent>();
	if (rb != nullptr)
	{
		rb->SetHorizontalVelocity(m_blackboard->currentVelocity);
	}
	if (m_blackboard->isLockedOn && m_blackboard->currentStance == HEIN::CombatStance::Strafing &&  m_blackboard->dirToTarget.LengthSquared() > 0.001f)
	{
		float targetYaw = atan2f(m_blackboard->dirToTarget.x, m_blackboard->dirToTarget.z);
		float modelOffset = DirectX::XM_PI;
		targetYaw += modelOffset;
		DirectX::SimpleMath::Vector3 currentRot = m_transform->GetRotationEuler();

		float difference = targetYaw - currentRot.y;
		while (difference < -DirectX::XM_PI) difference += DirectX::XM_2PI;
		while (difference > DirectX::XM_PI) difference -= DirectX::XM_2PI;

		float turnSpeed = 10.0f;
		currentRot.y += difference * turnSpeed * deltaTime;

		currentRot.x = 0.0f;
		currentRot.z = 0.0f;

		m_transform->SetRotationEuler(currentRot);
	}
	else if (m_blackboard->moveIntent.LengthSquared() > 0.001f)
	{
		float targetYaw = atan2f(m_blackboard->moveIntent.x, m_blackboard->moveIntent.z);
		float modelOffset = DirectX::XM_PI;
		targetYaw += modelOffset;

		DirectX::SimpleMath::Vector3 currentRot = m_transform->GetRotationEuler();

		float difference = targetYaw - currentRot.y;
		while (difference < -DirectX::XM_PI) difference += DirectX::XM_2PI;
		while (difference > DirectX::XM_PI) difference -= DirectX::XM_2PI;

		float turnSpeed = 10.0f;
		currentRot.y += difference * turnSpeed * deltaTime;

		currentRot.x = 0.0f;
		currentRot.z = 0.0f;

		m_transform->SetRotationEuler(currentRot);
	}
}
