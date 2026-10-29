#include "pch.h"
#include "BTChaseNode.h"
#include "BlackBoard/CombatBlackBoard.h"

namespace HEIN
{
BTChaseNode::BTChaseNode(float attackRange, float walkSpeed, bool succeedOnArrival)
	: m_attackRange(attackRange)
	, m_walkSpeed(walkSpeed)
	, m_succeedOnArrival(succeedOnArrival)
{
}

void BTChaseNode::OnEnter(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
{
	m_isActive = true;
	m_currentState = BTNodeState::Running;
	if (self)
	{
		auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
		if (blackboard)
		{
			blackboard->activeNodeName = GetNodeName();
			blackboard->isLockedOn = true;
		}
	}
}

void BTChaseNode::OnExit(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
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
	m_isActive = false;
}

void BTChaseNode::Abort(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID)
{
	OnExit(self, manager, targetID);
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

void BTChaseNode::Reset()
{
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

BTNodeState BTChaseNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float /*deltaTime*/)
{
	if (!self) return BTNodeState::Failure;

	auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (!blackboard) return BTNodeState::Failure;

	if (blackboard->isLeashing)
	{
		OnExit(self, manager, targetID);
		m_currentState = BTNodeState::Failure;
		return BTNodeState::Failure;
	}

	if (!m_isActive || m_currentState != BTNodeState::Running)
	{
		OnEnter(self, manager, targetID);
	}

	blackboard->activeNodeName = "Chase Node";

	if (blackboard->distanceToTarget <= m_attackRange)
	{
		blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
		blackboard->isLockedOn = true;
		blackboard->activeNodeName = "In Melee Range (Waiting)";

		OnExit(self, manager, targetID);
		BTNodeState resultState = m_succeedOnArrival ? BTNodeState::Success : BTNodeState::Failure;
		m_currentState = resultState;
		return resultState;
	}

	blackboard->isLockedOn = true;
	blackboard->moveIntent = blackboard->dirToTarget;
	blackboard->currentSpeed = m_walkSpeed;

	m_currentState = BTNodeState::Running;
	return BTNodeState::Running;
}
} // namespace HEIN
