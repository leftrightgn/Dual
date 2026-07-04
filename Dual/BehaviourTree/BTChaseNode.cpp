#include "pch.h"
#include "BTChaseNode.h"
#include <BlackBoard/CombatBlackBoard.h>

HEIN::BTChaseNode::BTChaseNode(float attackRange, float walkSpeed)
	: m_attackRange(attackRange)
	, m_walkSpeed(walkSpeed)
{
}

HEIN::BTNodeState HEIN::BTChaseNode::Tick(
	HEIN::Actor* self, 
	HEIN::ActorManager* manager, 
	HEIN::ActorID targetID, 
	float deltaTime
)
{
	HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;
	blackboard->lockOnIntent = true;
	blackboard->isStrafingIntent = false;
	if (blackboard->distanceToTarget <= m_attackRange)
	{
		blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		return BTNodeState::Success;
	}

	blackboard->moveIntent = blackboard->dirToTarget;
	blackboard->currentSpeed = m_walkSpeed;

	return BTNodeState::Running;
}
