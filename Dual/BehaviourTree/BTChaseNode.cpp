#include "pch.h"
#include "BTChaseNode.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Message/Message.h>
#include <Message/Messenger.h>

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
	blackboard->isLockedOn = false;
	Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_EXIT_STRAFE);
	if (blackboard->distanceToTarget <= m_attackRange)
	{
		blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		return BTNodeState::Success;
	}

	blackboard->moveIntent = blackboard->dirToTarget;
	blackboard->currentSpeed = m_walkSpeed;

	return BTNodeState::Running;
}
