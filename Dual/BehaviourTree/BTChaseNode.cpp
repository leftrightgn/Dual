#include "pch.h"
#include "BTChaseNode.h"
#include <BlackBoard/CombatBlackBoard.h>
#include "../../External/Engine/Message/Messenger.h" 
#include "../../External/Engine/Message/Message.h" 
#include "../../External/Engine/Components/TransformComponent.h"

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
	
	blackboard->activeNodeName = "Chase Node";
	
	blackboard->isLockedOn = false;
	if (blackboard->distanceToTarget <= m_attackRange)
	{
		blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;

		// Smoothly face the target when in close range
		if (blackboard->dirToTarget.LengthSquared() > 0.001f)
		{
			float targetYaw = atan2f(blackboard->dirToTarget.x, blackboard->dirToTarget.z) + DirectX::XM_PI;
			DirectX::SimpleMath::Quaternion targetRotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(targetYaw, 0.0f, 0.0f);
			if (auto* transform = self->GetComponent<HEIN::TransformComponent>())
			{
				DirectX::SimpleMath::Quaternion currentRot = transform->GetRotation();
				transform->SetRotation(DirectX::SimpleMath::Quaternion::Slerp(currentRot, targetRotation, deltaTime * 8.0f));
			}
		}

		return BTNodeState::Success;
	}

	blackboard->moveIntent = blackboard->dirToTarget;
	blackboard->currentSpeed = m_walkSpeed;

	return BTNodeState::Running;
}
