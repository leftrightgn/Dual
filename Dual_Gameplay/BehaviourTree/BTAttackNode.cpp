#include "pch.h"
#include "BTAttackNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "BlackBoard/BossBlackBoard.h"
#include "../../External/Engine/Components/TransformComponent.h"
#include "../../External/Engine/Message/Message.h"
#include "../../External/Engine/Message/Messenger.h"
#include <cmath>

namespace HEIN
{
HEIN::BTAttackNode::BTAttackNode(
	float attackDuration,
	float minAttackDistance,
	bool isHeavyAttack,
	float postAttackCooldown
)
	: m_attackDuration(attackDuration)
	, m_timer(0.0f)
	, m_minAttackDis(minAttackDistance)
	, m_isAttacking(false)
	, m_isHeavyAttack(isHeavyAttack)
	, m_postAttackCooldown(postAttackCooldown)
{
}

void BTAttackNode::OnEnter(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
{
	m_timer = 0.0f;
	m_isAttacking = false;
	m_isActive = true;
	m_currentState = BTNodeState::Running;

	if (!self) return;

	auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (!blackboard) return;

	blackboard->activeNodeName = GetNodeName();

	auto* bossBB = dynamic_cast<HEIN::BossBlackBoard*>(blackboard);
	if (bossBB && !m_isHeavyAttack)
	{
		bossBB->RollNextBossComboPattern();
		blackboard->activeNodeName = "Combo Attack [" + bossBB->activeComboPatternName + "]";
	}

	if (m_isHeavyAttack)
	{
		Messenger::GetInstance()->Notify(self->GetID(), Message::ENEMY_ACTION_HEAVY_ATTACK);
	}
	else
	{
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_ACTION_ATTACK);
	}

	m_isAttacking = true;
	blackboard->isAttacking = true;
	blackboard->isLockedOn = true;
}

void BTAttackNode::OnExit(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
{
	if (self)
	{
		auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
		if (blackboard)
		{
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->isAttacking = false;
			blackboard->isLockedOn = false;
		}
		Messenger::GetInstance()->Notify(self->GetID(), Message::SET_WEAPON_INACTIVE);
		Messenger::GetInstance()->Notify(self->GetID(), Message::RESET_WEAPON_SWING);
	}
	m_isAttacking = false;
	m_timer = 0.0f;
}

void BTAttackNode::Abort(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID)
{
	OnExit(self, manager, targetID);
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

void BTAttackNode::Reset()
{
	m_timer = 0.0f;
	m_isAttacking = false;
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

HEIN::BTNodeState HEIN::BTAttackNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	if (!self) return BTNodeState::Failure;

	auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (!blackboard) return BTNodeState::Failure;

	if (!m_isActive || m_currentState != BTNodeState::Running)
	{
		if (blackboard->distanceToTarget > m_minAttackDis)
		{
			m_isActive = false;
			m_currentState = BTNodeState::Failure;
			return BTNodeState::Failure;
		}
		OnEnter(self, manager, targetID);
	}

	if (blackboard->currentHealth <= 0.0f || blackboard->currentStance == HEIN::CombatStance::Staggered)
	{
		OnExit(self, manager, targetID);
		m_isActive = false;
		m_currentState = BTNodeState::Failure;
		return BTNodeState::Failure;
	}

	if (false)
	{
		OnExit(self, manager, targetID);
		m_isActive = false;
		m_currentState = BTNodeState::Failure;
		return BTNodeState::Failure;
	}

	float rawDt = (self->GetUnscaledDeltaTime() > 0.0f) ? self->GetUnscaledDeltaTime() : deltaTime;

	bool shouldTrackTarget = m_isHeavyAttack
		? (m_timer < (m_attackDuration * m_heavyTrackRatio))
		: (m_timer < m_comboTrackTime || blackboard->currentStance == HEIN::CombatStance::AttackWindup);

	blackboard->isLockedOn = shouldTrackTarget;

	if (m_isHeavyAttack)
	{
		if (m_timer < (m_attackDuration * m_heavyTrackRatio) && blackboard->distanceToTarget > m_lungeDistanceThreshold)
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

	m_timer += rawDt;

	auto* bossBB = dynamic_cast<HEIN::BossBlackBoard*>(blackboard);
	int currentComboIdx = bossBB ? bossBB->comboIndex : 0;
	bool isComplete = false;
	if (!m_isHeavyAttack && m_timer >= GameConstants::BossAI::EarlySwingMissTimeout && currentComboIdx == 0 && !blackboard->lastAttackHit) { isComplete = true; } else if (false)
	{
		OnExit(self, manager, targetID);
		m_isActive = false;
		m_currentState = BTNodeState::Success;
		return BTNodeState::Success;
	}

	if (m_timer > GameConstants::BossAI::AttackStateMinCheckTime && !blackboard->isAttacking &&
		blackboard->currentStance != HEIN::CombatStance::AttackRelese &&
		blackboard->currentStance != HEIN::CombatStance::AttackWindup) { isComplete = true; } else if (false)
	{
		OnExit(self, manager, targetID);
		m_isActive = false;
		m_currentState = BTNodeState::Success;
		return BTNodeState::Success;
	}

	if (m_timer >= m_attackDuration) { isComplete = true; } if (isComplete) { if (m_postAttackCooldown > 0.0f) blackboard->actionCooldownTimer = m_postAttackCooldown; } if (isComplete)
	{
		OnExit(self, manager, targetID);
		m_isActive = false;
		m_currentState = BTNodeState::Success;
		return BTNodeState::Success;
	}

	m_currentState = BTNodeState::Running;
	return BTNodeState::Running;
}
} // namespace HEIN
