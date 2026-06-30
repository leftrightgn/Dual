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
	HEIN::CombatBlackBoard* bb = self->GetComponent<HEIN::CombatBlackBoard>();
	if (bb == nullptr) return BTNodeState::Failure;

	if (bb->distanceToTarget <= m_attackRange)
	{
		bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		return BTNodeState::Success;
	}

	bb->moveIntent = bb->dirToTarget;
	bb->currentSpeed = m_walkSpeed;

	return BTNodeState::Running;
}
