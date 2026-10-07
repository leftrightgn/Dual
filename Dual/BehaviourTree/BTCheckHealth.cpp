#include "pch.h"
#include "BTCheckHealth.h"
#include <Components/HealthComponent.h>

HEIN::BTCheckHealth::BTCheckHealth(float minPct, float maxPct)
	: m_minPct(minPct)
	, m_maxPct(maxPct)
{
}

HEIN::BTNodeState HEIN::BTCheckHealth::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	auto* health = self->GetComponent<HEIN::HealthComponent>();
	if (!health) return BTNodeState::Failure;

	float current = static_cast<float>(health->GetCurrentHealth());
	float maxH = static_cast<float>(health->GetMaxHealth());

	float pct = current / maxH;
	if (pct >= (m_minPct - 0.001f) && pct <= (m_maxPct + 0.001f))
	{
		return BTNodeState::Success;
	}

	return BTNodeState::Failure;
}
