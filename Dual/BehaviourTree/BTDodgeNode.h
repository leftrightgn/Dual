#pragma once
#include "../../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"


namespace HEIN
{
	/**
	 * @brief BTDodgeNode is an Action Node handling defensive evasive maneuvers.
	 * 
	 * RESPONSIBILITY & DECOUPLING:
	 * Interfaces with the CombatStateMachine by injecting dodge intents and calculating an evasion vector.
	 * It reads `dodgeCooldownTimer` (or manages its own cooldown state) to prevent spamming, 
	 * and uses the CombatBlackBoard's `dirToTarget` to calculate a reverse or perpendicular roll direction.
	 * 
	 * STATE EVALUATION:
	 * - Success: Returns Success when the dodge animation finishes and the actor has recovered.
	 * - Running: Returns Running while the dodge animation/i-frames are active.
	 * - Failure: Returns Failure if the dodge is on cooldown, or if the target is invalid.
	 */
	class BTDodgeNode : public BTNode
	{
	private:

		float m_cooldown;
		float m_cooldownTimer;
		float m_dodgeStateTimer;
		float m_timeoutTimer;
		bool m_isDodging;
		bool m_hasDodgeStarted;

	public:

		BTDodgeNode(float cooldown = 3.0f);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
