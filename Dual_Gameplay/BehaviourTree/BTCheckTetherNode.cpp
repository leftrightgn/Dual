#include "pch.h"
#include "BTCheckTetherNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Components/TransformComponent.h"

namespace HEIN
{
	HEIN::BTCheckTetherNode::BTCheckTetherNode(float maxTetherDistance)
		: m_maxTetherDistance(maxTetherDistance)
	{
	}

	HEIN::BTNodeState HEIN::BTCheckTetherNode::Tick(
		HEIN::Actor* self, 
		HEIN::ActorManager* manager, 
		HEIN::ActorID targetID, 
		float /*deltaTime*/
	)
	{
		if (!self) return BTNodeState::Failure;

		HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
		if (!blackboard) return BTNodeState::Failure;

		// Once leashing has started, stay committed to returning to spawn until home is reached
		if (blackboard->isLeashing)
		{
			return BTNodeState::Success;
		}

		const float effectiveTether = (blackboard->aggroRange > 0.0f)
			? blackboard->aggroRange
			: m_maxTetherDistance;

		if (blackboard->hasSetSpawnPosition && effectiveTether > 0.0f)
		{
			if (auto* selfTransform = self->GetComponent<HEIN::TransformComponent>())
			{
				DirectX::SimpleMath::Vector3 selfToSpawn = blackboard->spawnPosition - selfTransform->GetPosition();
				selfToSpawn.y = 0.0f;
				if (selfToSpawn.Length() > effectiveTether)
				{
					blackboard->isLeashing = true;
					return BTNodeState::Success;
				}
			}
		}

		HEIN::Actor* targetActor = manager ? manager->GetActor(targetID) : nullptr;
		if (!targetActor) return BTNodeState::Failure;

		HEIN::TransformComponent* targetTransform = targetActor->GetComponent<HEIN::TransformComponent>();
		if (!targetTransform) return BTNodeState::Failure;

		DirectX::SimpleMath::Vector3 targetPos = targetTransform->GetPosition();
		DirectX::SimpleMath::Vector3 targetToSpawn = blackboard->spawnPosition - targetPos;
		targetToSpawn.y = 0.0f;

		if (effectiveTether > 0.0f && targetToSpawn.Length() > effectiveTether)
		{
			blackboard->isLeashing = true;
			return BTNodeState::Success;
		}

		return BTNodeState::Failure;
	}
} // namespace HEIN
