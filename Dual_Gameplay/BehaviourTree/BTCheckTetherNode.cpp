#include "pch.h"
#include "BTCheckTetherNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "BlackBoard/BossBlackBoard.h"
#include "Components/TransformComponent.h"

namespace HEIN
{
HEIN::BTCheckTetherNode::BTCheckTetherNode(float maxTetherDistance)
	: m_maxTetherDistance(maxTetherDistance)
{
}

HEIN::BTNodeState HEIN::BTCheckTetherNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float /*deltaTime*/)
{
	HEIN::BossBlackBoard* blackboard = self->GetComponent<HEIN::BossBlackBoard>();
	if (!blackboard) return BTNodeState::Failure;

	// Once leashing has started, stay committed to returning to spawn and ignore target range until home is reached
	if (blackboard->isLeashing)
	{
		return BTNodeState::Success;
	}

	if (blackboard->hasSetSpawnPosition && blackboard->aggroRange > 0.0f)
	{
		if (auto* selfTransform = self->GetComponent<HEIN::TransformComponent>())
		{
			DirectX::SimpleMath::Vector3 selfToSpawn = blackboard->spawnPosition - selfTransform->GetPosition();
			selfToSpawn.y = 0.0f;
			if (selfToSpawn.Length() > blackboard->aggroRange)
			{
				blackboard->isLeashing = true;
				return BTNodeState::Success;
			}
		}
	}

	HEIN::Actor* player = manager ? manager->GetActor(targetID) : nullptr;
	if (!player) return BTNodeState::Failure;

	HEIN::TransformComponent* playerTransform = player->GetComponent<HEIN::TransformComponent>();
	if (!playerTransform) return BTNodeState::Failure;

	DirectX::SimpleMath::Vector3 playerPos = playerTransform->GetPosition();
	DirectX::SimpleMath::Vector3 playerToSpawn = blackboard->spawnPosition - playerPos;
	playerToSpawn.y = 0.0f; // ignore vertical distance

	// Did the player get out of the aggro range?
	if (playerToSpawn.Length() > blackboard->aggroRange)
	{
		blackboard->isLeashing = true; // Reset target/position to spawn.
		return BTNodeState::Success;
	}

	return BTNodeState::Failure;
}
} // namespace HEIN
