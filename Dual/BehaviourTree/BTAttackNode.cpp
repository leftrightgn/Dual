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
	HEIN::BossBlackBoard* blackboard = self->GetComponent<HEIN::BossBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	float rawDt = (self && self->GetUnscaledDeltaTime() > 0.0f) ? self->GetUnscaledDeltaTime() : deltaTime;

	const std::string expectedPrefix = m_isHeavyAttack ? "Heavy Attack" : "Combo Attack";
	if (m_timer > 0.0f && blackboard->activeNodeName.find(expectedPrefix) == std::string::npos)
	{
		// Node was previously interrupted by a higher-priority branch (e.g. Dodge); reset timer
		m_timer = 0.0f;
	}

	if (m_timer == 0.0f)
	{
		if (blackboard->distanceToTarget > m_minAttackDis)
		{
			return BTNodeState::Failure;
		}

		if (!m_isHeavyAttack)
		{
			// Roll next 3-hit combo permutation (123, 132, 213, 231, 312, 321)
			blackboard->RollNextBossComboPattern();
		}
	}

	blackboard->activeNodeName = m_isHeavyAttack
		? "Heavy Attack Node"
		: ("Combo Attack [" + blackboard->activeComboPatternName + "]");

	// Rotational tracking:
	// - Heavy jump attack: track up to 60% of animation so the leap aims directly at the player.
	// - 3-Hit Combo: re-acquire target angle during the wind-up phase of each of the 3 swings!
	bool shouldTrackTarget = m_isHeavyAttack
		? (m_timer < (m_attackDuration * m_heavyTrackRatio))
		: (m_timer < m_comboTrackTime || blackboard->currentStance == HEIN::CombatStance::AttackWindup);

	if (shouldTrackTarget)
	{
		// Calculate desired rotation to face the target directly.
		// Adds PI to yaw because standard orientation might be reversed depending on the model's forward vector.
		float targetYaw = atan2f(blackboard->dirToTarget.x, blackboard->dirToTarget.z) + DirectX::XM_PI;
		DirectX::SimpleMath::Quaternion targetRotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(targetYaw, 0.0f, 0.0f);

		HEIN::TransformComponent* transform = self->GetComponent<HEIN::TransformComponent>();
		if (transform != nullptr)
		{
			DirectX::SimpleMath::Quaternion currentRot = transform->GetRotation();

			float turnSpeed = m_isHeavyAttack ? m_heavyTurnSpeed : m_comboTurnSpeed;

			DirectX::SimpleMath::Quaternion newRot = DirectX::SimpleMath::Quaternion::Slerp(currentRot, targetRotation, rawDt * turnSpeed);
			transform->SetRotation(newRot);
		}
	}

	// Forward combat step during wind-up:
	// For heavy attack: the jump animation naturally propels the boss 15-18 units forward.
	// For 3-hit combo: step forward during each swing's wind-up if the player is > 20.0f away.
	if (m_isHeavyAttack)
	{
		if (m_timer < (m_attackDuration * m_comboTrackTime) && blackboard->distanceToTarget > m_lungeDistanceThreshold)
		{
			blackboard->moveIntent = blackboard->dirToTarget;
			blackboard->currentSpeed = m_lungeSpeed;
		}
		else
		{
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		}
	}
	else
	{
		if (shouldTrackTarget && blackboard->distanceToTarget > m_stepDistanceThreshold)
		{
			blackboard->moveIntent = blackboard->dirToTarget;
			blackboard->currentSpeed = m_stepSpeed;
		}
		else
		{
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		}
	}

	// Send initial attack trigger on node entry; subsequent combo steps (2nd & 3rd swings)
	// advance in UniversalCombatState::Update to deliver a relentless 3-hit Elden Ring combo!
	if (m_timer == 0.0f)
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

	m_timer += rawDt;

	// If the first swing missed, don't wait for the full 3-hit duration — finish and chase!
	if (!m_isHeavyAttack && m_timer >= GameConstants::BossAI::EarlySwingMissTimeout && blackboard->comboIndex == 0 && !blackboard->lastAttackHit)
	{
		m_timer = 0.0f;
		return BTNodeState::Success;
	}

	// If the state machine is no longer in an attack state (full 3-hit combo or heavy attack completed),
	// finish the node so the AI transitions smoothly to its next decision (e.g. Chase or next combo).
	if (m_timer > GameConstants::BossAI::AttackStateMinCheckTime && !blackboard->isAttacking &&
		blackboard->currentStance != HEIN::CombatStance::AttackRelese &&
		blackboard->currentStance != HEIN::CombatStance::AttackWindup)
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