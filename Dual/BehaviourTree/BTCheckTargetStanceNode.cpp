#include "pch.h"
#include "BTCheckTargetStanceNode.h"

HEIN::BTCheckTargetStanceNode::BTCheckTargetStanceNode(CombatStance desiredStance)
	: m_desiredStance(desiredStance)
	, m_checkIsAttacking(false)
{
}

HEIN::BTCheckTargetStanceNode::BTCheckTargetStanceNode(bool checkIsAttacking)
	: m_desiredStance(CombatStance::Idle)
	, m_checkIsAttacking(checkIsAttacking)
{
}

HEIN::BTNodeState HEIN::BTCheckTargetStanceNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (!blackboard) return BTNodeState::Failure;

	if (m_checkIsAttacking)
	{
		return blackboard->isTargetAttacking ? BTNodeState::Success : BTNodeState::Failure;
	}

	return (blackboard->targetStance == m_desiredStance) ? BTNodeState::Success : BTNodeState::Failure;

}
