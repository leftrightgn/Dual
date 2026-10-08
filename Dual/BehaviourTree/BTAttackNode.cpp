#include "pch.h"
#include "BTAttackNode.h"
#include <BlackBoard/CombatBlackBoard.h>
#include "../../External/Engine/Message/Messenger.h" 
#include "../../External/Engine/Message/Message.h" 
#include <cmath>
#include "../../External/Engine/Components/TransformComponent.h"


HEIN::BTAttackNode::BTAttackNode(
	float attackDuration,
	float minAttackDistance,
	bool isHeavyAttack
)
	: m_attackDuration(attackDuration)
	, m_timer(0.0f)
	, m_minAttackDis(minAttackDistance)
	, m_isAttacking(false)
	, m_isHeavyAttack(isHeavyAttack)
{
}

HEIN::BTNodeState HEIN::BTAttackNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	blackboard->activeNodeName = m_isHeavyAttack ? "Heavy Attack Node" : "Combo Attack Node";

	if (m_timer == 0.0f && blackboard->distanceToTarget > m_minAttackDis)
	{
		return BTNodeState::Failure;
	}

	// Rotational tracking: for heavy jump attack, continue tracking up to 60% of animation so the leap aims directly at the player
	float trackingWindow = m_isHeavyAttack ? (m_attackDuration * 0.60f) : (m_attackDuration * 0.35f);
	if (m_timer < trackingWindow)
	{
		// Calculate desired rotation to face the target directly.
		// Adds PI to yaw because standard orientation might be reversed depending on the model's forward vector.
		float targetYaw = atan2f(blackboard->dirToTarget.x, blackboard->dirToTarget.z) + DirectX::XM_PI;
		DirectX::SimpleMath::Quaternion targetRotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(targetYaw, 0.0f, 0.0f);

		HEIN::TransformComponent* transform = self->GetComponent<HEIN::TransformComponent>();
		if (transform != nullptr)
		{
			DirectX::SimpleMath::Quaternion currentRot = transform->GetRotation();

			float turnSpeed = m_isHeavyAttack ? 8.0f : 6.0f;

			DirectX::SimpleMath::Quaternion newRot = DirectX::SimpleMath::Quaternion::Slerp(currentRot, targetRotation, deltaTime * turnSpeed);
			transform->SetRotation(newRot);
		}
	}

	// Forward combat step during initial wind-up:
	// For heavy attack: the jump animation naturally propels the boss 15-18 units forward.
	// Only step forward if target is far away (> 20.0f) to avoid overshooting and landing behind the player.
	if (m_timer < (m_attackDuration * 0.35f))
	{
		if (m_isHeavyAttack)
		{
			if (blackboard->distanceToTarget > 22.0f)
			{
				blackboard->moveIntent = blackboard->dirToTarget;
				blackboard->currentSpeed = 35.0f;
			}
			else
			{
				blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			}
		}
		else
		{
			// Regular combo attack: close in slightly if beyond weapon reach (maintains generous spacing >= 20.0f)
			if (blackboard->distanceToTarget > 20.0f)
			{
				blackboard->moveIntent = blackboard->dirToTarget;
				blackboard->currentSpeed = 30.0f;
			}
			else
			{
				blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			}
		}
	}
	else
	{
		blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
	}

	// Send attack inputs periodically so the FSM can buffer them for combos
	if (std::fmod(m_timer, 0.4f) < deltaTime || m_timer == 0.0f)
	{
		if (m_isHeavyAttack)
		{
			Messenger::GetInstance()->Notify(self->GetID(), Message::ENEMY_ACTION_HEAVY_ATTACK);
		}
		else
		{
			Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_ACTION_ATTACK);
		}
	}

	m_timer += deltaTime;

	// Smart combo abort: if combo 1 missed, don't stand idle waiting for a 3-hit duration!
	if (!m_isHeavyAttack && m_timer >= 1.25f && !blackboard->lastAttackHit)
	{
		m_timer = 0.0f;
		return BTNodeState::Success;
	}

	// If the state machine is no longer in an attack state (combo completed or recovered to Idle),
	// finish the node early so the AI doesn't stand frozen waiting for the full m_attackDuration.
	if (m_timer > 0.4f && !blackboard->isAttacking && blackboard->currentStance != HEIN::CombatStance::AttackRelese)
	{
		m_timer = 0.0f;
		return BTNodeState::Success;
	}

	if (m_timer >= m_attackDuration)
	{
		m_timer = 0.0f;
		return BTNodeState::Success;
	}

	return BTNodeState::Running;
}