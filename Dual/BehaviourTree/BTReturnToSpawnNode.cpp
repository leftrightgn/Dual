#include "pch.h"
#include "BTReturnToSpawnNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Components/CombatStateMachineComponent.h"
#include "Components/TransformComponent.h"
#include "Message/Message.h"
#include "Message/Messenger.h"

HEIN::BTReturnToSpawnNode::BTReturnToSpawnNode(float walkSpeed)
	: m_walkSpeed(walkSpeed)
{
}

HEIN::BTNodeState HEIN::BTReturnToSpawnNode::Tick(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/, float /*deltaTime*/)
{
	HEIN::BossBlackBoard* blackboard = self->GetComponent<HEIN::BossBlackBoard>();
	HEIN::TransformComponent* transform = self->GetComponent<HEIN::TransformComponent>();

	if (blackboard == nullptr || transform == nullptr) return BTNodeState::Failure;

	DirectX::SimpleMath::Vector3 currentPos = transform->GetPosition();
	DirectX::SimpleMath::Vector3 dirToHome = blackboard->spawnPosition - currentPos;

	// Evaluate horizontal distance on the XZ plane, ignoring vertical displacement
	dirToHome.y = 0.0f;
	float distanceToHome = dirToHome.Length();

	// If reached, succeed this node to prevent falling back to combat.
	if (distanceToHome < 2.0f)
	{
		blackboard->activeNodeName = "Idle at Spawn";
		blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
		blackboard->isLockedOn = false;
		
		// Wait at home.
		return BTNodeState::Success;
	}

	// Far from spawn; return to spawn position.
	blackboard->activeNodeName = "Returning to Spawn";
	blackboard->isLockedOn = false;
	Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_EXIT_STRAFE);

	dirToHome.Normalize();
	blackboard->moveIntent = dirToHome;
	blackboard->currentSpeed = m_walkSpeed;

	return BTNodeState::Running;
}