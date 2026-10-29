#include "pch.h"
#include "BTIdleNode.h"
#include "BlackBoard/CombatBlackBoard.h"

namespace HEIN
{
HEIN::BTNodeState HEIN::BTIdleNode::Tick(
	HEIN::Actor* self, 
	HEIN::ActorManager* /*manager*/, 
	HEIN::ActorID /*targetID*/, 
	float /*deltaTime*/
)
{
	if (!self) return BTNodeState::Failure;

	HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	blackboard->activeNodeName = "Idle / Waiting";
	blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
	blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
	blackboard->isLockedOn = false;

	return BTNodeState::Running;
}

void HEIN::BTIdleNode::Abort(
	HEIN::Actor* self, 
	HEIN::ActorManager* manager, 
	HEIN::ActorID targetID
)
{
	if (self)
	{
		if (auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>())
		{
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
		}
	}
	BTNode::Abort(self, manager, targetID);
}

void HEIN::BTIdleNode::Reset()
{
	BTNode::Reset();
}
} // namespace HEIN
