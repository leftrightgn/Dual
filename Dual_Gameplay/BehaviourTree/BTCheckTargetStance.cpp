#include "pch.h"
#include "BTCheckTargetStance.h"
#include "BlackBoard/BossBlackBoard.h"

namespace HEIN
{
HEIN::BTCheckTargetStance::BTCheckTargetStance(CombatStance desiredStance)
	: m_desiredStance(desiredStance)
	, m_checkIsAttacking(false)
{
}

HEIN::BTCheckTargetStance::BTCheckTargetStance(bool checkIsAttacking)
	: m_desiredStance(CombatStance::Idle)
	, m_checkIsAttacking(checkIsAttacking)
{
}

HEIN::BTNodeState HEIN::BTCheckTargetStance::Tick(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/, float /*deltaTime*/)
{
	auto* blackboard = self->GetComponent<HEIN::BossBlackBoard>();
	if (!blackboard) return BTNodeState::Failure;

	if (m_checkIsAttacking)
	{
		// Keep reactive dodge sequence latched while the actor is actively mid-dodge
		if (blackboard->currentStance == CombatStance::Dodging)
		{
			return BTNodeState::Success;
		}
		return blackboard->isTargetAttacking ? BTNodeState::Success : BTNodeState::Failure;
	}

	return (blackboard->targetStance == m_desiredStance) ? BTNodeState::Success : BTNodeState::Failure;
}
} // namespace HEIN
