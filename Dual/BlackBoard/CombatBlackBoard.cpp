#include "pch.h"
#include "CombatBlackBoard.h"
#include <Components/GaugeComponent.h>

// ============================================================================
// Update: Push Dodge/Block values to GaugeComponent if present on owner actor.
// HP is handled by GaugeComponent::SyncBoundBars (engine-side pull).
// ============================================================================
void HEIN::CombatBlackBoard::Update(float /*deltaTime*/)
{
    auto* gauge = m_owner->GetComponent<GaugeComponent>();
    if (!gauge) return;

    for (int i = 0; i < gauge->GetBarCount(); i++)
    {
        GaugeBarDef* bar = gauge->GetBar(i);
        if (!bar || !bar->autoBind) continue;

        if (bar->gaugeType == GaugeType::Block)
        {
            bar->currentValue = currentBlockStamina;
            bar->maxValue = maxBlockStamina;
        }
        else if (bar->gaugeType == GaugeType::Dodge)
        {
            // Inverted: show readiness (full = ready, empty = on cooldown)
            bar->currentValue = (dodgeCooldownTimer > 0.0f)
                ? (maxDodgeCooldown - dodgeCooldownTimer)
                : maxDodgeCooldown;
            bar->maxValue = maxDodgeCooldown;
        }
    }
}
