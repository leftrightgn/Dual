#include "pch.h"
#include "BTReturnToSpawnNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "BlackBoard/BossBlackBoard.h"
#include "Components/CombatStateMachineComponent.h"
#include "Components/TransformComponent.h"
#include "Message/Message.h"
#include "Message/Messenger.h"

namespace HEIN
{
HEIN::BTReturnToSpawnNode::BTReturnToSpawnNode(float walkSpeed)
	: m_walkSpeed(walkSpeed)
{
}

HEIN::BTNodeState HEIN::BTReturnToSpawnNode::Tick(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/, float deltaTime)
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
		blackboard->isLeashing = false;
		
		// Wait at home.
		return BTNodeState::Success;
	}

	// Far from spawn; return to spawn position.
	blackboard->activeNodeName = "Returning to Spawn";
	blackboard->isLockedOn = false;
	blackboard->isAttacking = false;
	Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_EXIT_STRAFE);

	dirToHome.Normalize();
	blackboard->moveIntent = dirToHome;
	blackboard->currentSpeed = m_walkSpeed;

	float targetYaw = std::atan2(dirToHome.x, dirToHome.z) + DirectX::XM_PI;
	DirectX::SimpleMath::Quaternion targetRotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(targetYaw, 0.0f, 0.0f);
	DirectX::SimpleMath::Quaternion currentRot = transform->GetRotation();
	transform->SetRotation(DirectX::SimpleMath::Quaternion::Slerp(currentRot, targetRotation, std::clamp(deltaTime * 10.0f, 0.0f, 1.0f)));

	return BTNodeState::Running;
}
} // namespace HEIN
