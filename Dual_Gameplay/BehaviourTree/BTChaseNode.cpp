#include "pch.h"
#include "BTChaseNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "BlackBoard/BossBlackBoard.h"
#include "../../External/Engine/Components/TransformComponent.h"
#include "../../External/Engine/Message/Message.h"
#include "../../External/Engine/Message/Messenger.h"

namespace HEIN
{
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
	HEIN::BossBlackBoard* blackboard = self->GetComponent<HEIN::BossBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	// Yield immediately if tether/aggro range is exceeded so the root selector transitions to Leashing
	if (blackboard->isLeashing)
	{
		return BTNodeState::Failure;
	}
	if (blackboard->hasSetSpawnPosition && blackboard->aggroRange > 0.0f)
	{
		if (auto* selfTrans = self->GetComponent<HEIN::TransformComponent>())
		{
			DirectX::SimpleMath::Vector3 selfToSpawn = blackboard->spawnPosition - selfTrans->GetPosition();
			selfToSpawn.y = 0.0f;
			if (selfToSpawn.Length() > blackboard->aggroRange)
			{
				blackboard->isLeashing = true;
				return BTNodeState::Failure;
			}
		}
		if (HEIN::Actor* target = manager ? manager->GetActor(targetID) : nullptr)
		{
			if (auto* targetTrans = target->GetComponent<HEIN::TransformComponent>())
			{
				DirectX::SimpleMath::Vector3 targetToSpawn = blackboard->spawnPosition - targetTrans->GetPosition();
				targetToSpawn.y = 0.0f;
				if (targetToSpawn.Length() > blackboard->aggroRange)
				{
					blackboard->isLeashing = true;
					return BTNodeState::Failure;
				}
			}
		}
	}
	
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
} // namespace HEIN
