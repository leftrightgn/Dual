#include "pch.h"
#include "BTCheckTargetStance.h"
#include "BlackBoard/CombatBlackBoard.h"

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

HEIN::BTNodeState HEIN::BTCheckTargetStance::Tick(
	HEIN::Actor* self, 
	HEIN::ActorManager* /*manager*/, 
	HEIN::ActorID /*targetID*/, 
	float /*deltaTime*/
)
{
	if (!self) return BTNodeState::Failure;

	auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (!blackboard) return BTNodeState::Failure;

	if (m_checkIsAttacking)
	{
		return blackboard->isTargetAttacking ? BTNodeState::Success : BTNodeState::Failure;
	}

	return (blackboard->targetStance == m_desiredStance) ? BTNodeState::Success : BTNodeState::Failure;
}
} // namespace HEIN
