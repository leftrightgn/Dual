#include "pch.h"
#include "BTIdleNode.h"
#include "BlackBoard/CombatBlackBoard.h"

HEIN::BTNodeState HEIN::BTIdleNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	// Tell the debugger what the AI is thinking
	blackboard->activeNodeName = "Idle / Waiting";

	// Clear movement and lock-on so the enemy stops walking
	blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
	blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
	blackboard->isLockedOn = false;

	return BTNodeState::Success;
}
