#include "pch.h"
#include "BTCheckHealth.h"
#include "Components/HealthComponent.h"
#include "BlackBoard/CombatBlackBoard.h"

namespace HEIN
{
	HEIN::BTCheckHealth::BTCheckHealth(float minPct, float maxPct)
		: m_minPct(minPct)
		, m_maxPct(maxPct)
	{
	}

	HEIN::BTNodeState HEIN::BTCheckHealth::Tick(
		HEIN::Actor* self, 
		HEIN::ActorManager* /*manager*/, 
		HEIN::ActorID /*targetID*/, 
		float /*deltaTime*/
	)
	{
		if (!self) return BTNodeState::Failure;

		float current = 0.0f;
		float maxH = 0.0f;

		if (auto* health = self->GetComponent<HEIN::HealthComponent>())
		{
			current = static_cast<float>(health->GetCurrentHealth());
			maxH = static_cast<float>(health->GetMaxHealth());
		}
		else if (auto* combatBB = self->GetComponent<HEIN::CombatBlackBoard>())
		{
			current = combatBB->currentHealth;
			maxH = combatBB->maxHealth;
		}
		else
		{
			return BTNodeState::Failure;
		}

		if (maxH <= 0.0f) return BTNodeState::Failure;

		float pct = current / maxH;

		if (pct >= (m_minPct - 0.001f) && pct <= (m_maxPct + 0.001f))
		{
			return BTNodeState::Success;
		}

		return BTNodeState::Failure;
	}
} // namespace HEIN
