#include "pch.h"
#include "BTCooldownDecorator.h"
#include "BlackBoard/CombatBlackBoard.h"

namespace HEIN
{
BTCooldownDecorator::BTCooldownDecorator(float cooldownTime, std::unique_ptr<BTNode> child)
	: m_cooldownTime(cooldownTime)
	, m_cooldownExpirationTime(0.0f)
	, m_fallbackTimer(0.0f)
	, m_child(std::move(child))
{
}

void BTCooldownDecorator::OnEnter(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID)
{
	m_isActive = true;
	if (m_child)
	{
		m_child->OnEnter(self, manager, targetID);
	}
}

void BTCooldownDecorator::OnExit(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID)
{
	if (m_child)
	{
		m_child->OnExit(self, manager, targetID);
	}
	m_isActive = false;
}

void BTCooldownDecorator::Abort(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID)
{
	if (m_child && (m_child->IsActive() || m_child->IsRunning()))
	{
		m_child->Abort(self, manager, targetID);

		auto* blackboard = self ? self->GetComponent<HEIN::CombatBlackBoard>() : nullptr;
		if (blackboard)
		{
			m_cooldownExpirationTime = blackboard->elapsedCombatTime + m_cooldownTime;
		}
		else
		{
			m_fallbackTimer = m_cooldownTime;
		}
	}
	BTNode::Abort(self, manager, targetID);
}

void BTCooldownDecorator::Reset()
{
	if (m_child)
	{
		m_child->Reset();
	}
	BTNode::Reset();
}

BTNodeState BTCooldownDecorator::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	if (!m_child || !self) return BTNodeState::Failure;

	auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard)
	{
		if (blackboard->elapsedCombatTime < m_cooldownExpirationTime)
		{
			m_currentState = BTNodeState::Failure;
			return BTNodeState::Failure;
		}

		if (!m_isActive)
		{
			OnEnter(self, manager, targetID);
		}

		BTNodeState state = m_child->Tick(self, manager, targetID, deltaTime);
		m_currentState = state;

		if (state == BTNodeState::Success || state == BTNodeState::Failure)
		{
			m_cooldownExpirationTime = blackboard->elapsedCombatTime + m_cooldownTime;
			OnExit(self, manager, targetID);
		}
		return state;
	}

	if (m_fallbackTimer > 0.0f)
	{
		m_fallbackTimer -= deltaTime;
		m_currentState = BTNodeState::Failure;
		return BTNodeState::Failure;
	}

	if (!m_isActive)
	{
		OnEnter(self, manager, targetID);
	}

	BTNodeState state = m_child->Tick(self, manager, targetID, deltaTime);
	m_currentState = state;

	if (state == BTNodeState::Success || state == BTNodeState::Failure)
	{
		m_fallbackTimer = m_cooldownTime;
		OnExit(self, manager, targetID);
	}
	return state;
}
} // namespace HEIN
