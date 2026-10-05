#include "pch.h"
#include "BTCheckHealthNode.h"
#include <Components/HealthComponent.h>

HEIN::BTCheckHealthNode::BTCheckHealthNode(float minPct, float maxPct)
	: m_minPct(minPct)
	, m_maxPct(maxPct)
{
}

HEIN::BTNodeState HEIN::BTCheckHealthNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	auto* health = self->GetComponent<HEIN::HealthComponent>();
	if (!health) return BTNodeState::Failure;

	float current = static_cast<float>(health->GetCurrentHealth());
	float maxH = static_cast<float>(health->GetMaxHealth());

	float pct = current / maxH;
	if (pct >= m_minPct && pct <= m_maxPct)
	{
		return BTNodeState::Success;
	}

	return BTNodeState::Failure;
}
