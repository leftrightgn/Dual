#include "pch.h"
#include "BTCheckDistance.h"
#include <BlackBoard/CombatBlackBoard.h>

HEIN::BTCheckDistance::BTCheckDistance(
	float minDistance, 
	float maxDistance
)
	: m_minDistance(minDistance)
	, m_maxDistance(maxDistance)
{
}

HEIN::BTNodeState HEIN::BTCheckDistance::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	if (blackboard->distanceToTarget >= m_minDistance && blackboard->distanceToTarget <= m_maxDistance)
	{
		return BTNodeState::Success;
	}

	return BTNodeState::Failure;
}
