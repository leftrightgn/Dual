#include "pch.h"
#include "BTAttackNode.h"
#include <BlackBoard/CombatBlackBoard.h>
#include "../../External/Engine/Message/Messenger.h" 
#include "../../External/Engine/Message/Message.h" 
#include <cmath>
#include "../../External/Engine/Components/TransformComponent.h"


HEIN::BTAttackNode::BTAttackNode(
	float attackDuration,
	float minAttackDistance
)
	: m_attackDuration(attackDuration)
	, m_timer(0.0f)
	, m_minAttackDis(minAttackDistance)
	, m_isAttacking(false)
{
}

HEIN::BTNodeState HEIN::BTAttackNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	blackboard->activeNodeName = "Attack Node";

	if (m_timer == 0.0f && blackboard->distanceToTarget < m_minAttackDis)
	{
		return BTNodeState::Failure;
	}

	blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;

	float targetYaw = atan2f(blackboard->dirToTarget.x, blackboard->dirToTarget.z) + DirectX::XM_PI;
	DirectX::SimpleMath::Quaternion targetRotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(targetYaw, 0.0f, 0.0f);

	HEIN::TransformComponent* transform = self->GetComponent<HEIN::TransformComponent>();
	if (transform != nullptr)
	{
		DirectX::SimpleMath::Quaternion currentRot = transform->GetRotation();

		float turnSpeed = 5.0f;

		DirectX::SimpleMath::Quaternion newRot = DirectX::SimpleMath::Quaternion::Slerp(targetRotation, targetRotation, deltaTime * turnSpeed);
		transform->SetRotation(newRot);
	}

	
	// Send attack inputs periodically so the FSM can buffer them for combos
	if (std::fmod(m_timer, 0.5f) < deltaTime)
	{
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_ACTION_ATTACK);
	}

	m_timer += deltaTime;

	if (m_timer >= m_attackDuration)
	{
		m_timer = 0;
		return BTNodeState::Success;
	}


	return BTNodeState::Running;
}