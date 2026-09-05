#include "pch.h"
#include "BTCheckTetherNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Components/TransformComponent.h"

HEIN::BTNodeState HEIN::BTCheckTetherNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	HEIN::Actor* player = manager->GetActor(targetID);
	if (!player) return BTNodeState::Failure;

	HEIN::TransformComponent* playerTransform = player->GetComponent<HEIN::TransformComponent>();
	if (!playerTransform) return BTNodeState::Failure;

	DirectX::SimpleMath::Vector3 playerPos = playerTransform->GetPosition();
	DirectX::SimpleMath::Vector3 playerToSpawn = blackboard->spawnPosition - playerPos;
	playerToSpawn.y = 0.0f; // ignore vertical distance

	// Did the player get out of the aggro range?
	if (playerToSpawn.Length() > blackboard->aggroRange)
	{
		blackboard->isLeashing = true; // Snap the leash!
	}
	else
	{
		blackboard->isLeashing = false; // Player is in range, awake!
	}

	// If the leash is snapped, we MUST return success to force them to walk home.
	if (blackboard->isLeashing)
	{
		return BTNodeState::Success;
	}

	return BTNodeState::Failure;
}
