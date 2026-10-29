#include "pch.h"
#include "BTDodgeNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Components/TransformComponent.h"
#include "../../External/Engine/Message/Message.h"
#include "../../External/Engine/Message/Messenger.h"

namespace HEIN
{
BTDodgeNode::BTDodgeNode(float cooldown)
	: m_cooldown(cooldown)
	, m_cooldownTimer(0.0f)
	, m_dodgeStateTimer(0.0f)
	, m_timeoutTimer(0.0f)
	, m_isDodging(false)
	, m_hasDodgeStarted(false)
{
}

void BTDodgeNode::OnEnter(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
{
	m_isDodging = false;
	m_hasDodgeStarted = false;
	m_dodgeStateTimer = 0.0f;
	m_timeoutTimer = 0.0f;
	m_isActive = true;
	m_currentState = BTNodeState::Running;

	if (self)
	{
		auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
		if (blackboard)
		{
			blackboard->activeNodeName = GetNodeName();
			blackboard->isLockedOn = false;
		}
	}
}

void BTDodgeNode::OnExit(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
{
	if (self)
	{
		auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
		if (blackboard)
		{
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
		}
	}
	m_isDodging = false;
	m_hasDodgeStarted = false;
	m_timeoutTimer = 0.0f;
	m_isActive = false;
}

void BTDodgeNode::Abort(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID)
{
	OnExit(self, manager, targetID);
	m_cooldownTimer = m_cooldown;
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

void BTDodgeNode::Reset()
{
	m_isDodging = false;
	m_hasDodgeStarted = false;
	m_dodgeStateTimer = 0.0f;
	m_timeoutTimer = 0.0f;
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

BTNodeState BTDodgeNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	if (!self) return BTNodeState::Failure;

	auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (!blackboard) return BTNodeState::Failure;

	if (m_cooldownTimer > 0.0f)
	{
		m_cooldownTimer -= deltaTime;
		if (!m_isDodging)
		{
			m_currentState = BTNodeState::Failure;
			return BTNodeState::Failure;
		}
	}

	if (blackboard->dodgeCooldownTimer > 0.0f && !m_isDodging)
	{
		m_currentState = BTNodeState::Failure;
		return BTNodeState::Failure;
	}

	if (!m_isActive || m_currentState != BTNodeState::Running)
	{
		OnEnter(self, manager, targetID);
	}

	blackboard->activeNodeName = "Dodge Node";

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
	blackboard->localMoveIntent = DirectX::SimpleMath::Vector3(0.0f, 0.0f, -1.0f);
	blackboard->isLockedOn = false;

	if (!m_isDodging)
	{
		m_isDodging = true;
		m_dodgeStateTimer = 0.0f;
		m_hasDodgeStarted = false;
		m_timeoutTimer = 0.0f;
	}

	m_timeoutTimer += deltaTime;

	if (!m_hasDodgeStarted)
	{
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_ACTION_DODGE);
		if (blackboard->currentStance == CombatStance::Dodging)
		{
			m_hasDodgeStarted = true;
		}
		else if (m_timeoutTimer > 0.25f)
		{
			OnExit(self, manager, targetID);
			m_cooldownTimer = m_cooldown;
			m_currentState = BTNodeState::Failure;
			return BTNodeState::Failure;
		}
		m_currentState = BTNodeState::Running;
		return BTNodeState::Running;
	}

	if (m_hasDodgeStarted)
	{
		if (blackboard->currentStance != CombatStance::Dodging)
		{
			OnExit(self, manager, targetID);
			m_cooldownTimer = m_cooldown;
			m_currentState = BTNodeState::Success;
			return BTNodeState::Success;
		}
		m_currentState = BTNodeState::Running;
		return BTNodeState::Running;
	}

	m_currentState = BTNodeState::Running;
	return BTNodeState::Running;
}
} // namespace HEIN
