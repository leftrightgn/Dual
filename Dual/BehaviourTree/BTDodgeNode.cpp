#include "pch.h"
#include "BTDodgeNode.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/TransformComponent.h>
#include "../../External/Engine/Message/Messenger.h" 
#include "../../External/Engine/Message/Message.h"  


HEIN::BTDodgeNode::BTDodgeNode(float cooldown)
	: m_cooldown(cooldown)
	, m_cooldownTimer(0.0f)
	, m_dodgeStateTimer(0.0f)
	, m_timeoutTimer(0.0f)
	, m_isDodging(false)
	, m_hasDodgeStarted(false)
{
}

HEIN::BTNodeState HEIN::BTDodgeNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	HEIN::BossBlackBoard* blackboard = self->GetComponent<HEIN::BossBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	blackboard->activeNodeName = "Dodge Node";

	if (m_cooldownTimer > 0.0f)
	{
		m_cooldownTimer -= deltaTime;

		if (!m_isDodging) return BTNodeState::Failure;
	}

	if (blackboard->dodgeCooldownTimer > 0.0f)
	{
		if (!m_isDodging) return BTNodeState::Failure;
	}

	if (!m_isDodging)
	{
		m_isDodging = true;
		m_dodgeStateTimer = 0.0f;

		m_hasDodgeStarted = false;
		m_timeoutTimer = 0.0f;
	}

	m_timeoutTimer += deltaTime;

	DirectX::SimpleMath::Vector3 evadeDir = -blackboard->dirToTarget;
	evadeDir.y = 0.0f;
	if (evadeDir.LengthSquared() > 0.001f)
	{
		evadeDir.Normalize();
	}
	else
	{
		auto* trans = self->GetComponent<TransformComponent>();
		evadeDir = trans ? -trans->GetForward() : DirectX::SimpleMath::Vector3(0.0f, 0.0f, -1.0f);
		evadeDir.y = 0.0f;
		if (evadeDir.LengthSquared() > 0.001f) evadeDir.Normalize();
	}
	blackboard->moveIntent = evadeDir;

	if (!m_hasDodgeStarted)
	{
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_ACTION_DODGE);
		if (blackboard->currentStance == CombatStance::Dodging)
		{
			m_hasDodgeStarted = true;
			float dodgeSpeed = (blackboard->currentSpeed > 10.0f) ? blackboard->currentSpeed : 25.0f;
			blackboard->currentVelocity = evadeDir * dodgeSpeed;
		}
		else if (m_timeoutTimer > 0.25f)
		{
			// Quick fallback timeout to prevent getting stuck in dodge state
			// if the animation/stance transition fails to fire
			m_isDodging = false;
			m_cooldownTimer = m_cooldown;
			return BTNodeState::Failure;
		}
		return BTNodeState::Running;
	}
	if (m_hasDodgeStarted)
	{
		if (blackboard->currentStance != CombatStance::Dodging)
		{
			m_isDodging = false;
			m_hasDodgeStarted = false;
			m_cooldownTimer = m_cooldown;
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;

			return BTNodeState::Success;
		}
		return BTNodeState::Running;
	}
	return BTNodeState::Running;

}
