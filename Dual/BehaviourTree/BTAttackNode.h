#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"

namespace HEIN
{
	/**
	 * @brief BTAttackNode is an Action Node responsible for executing and timing an attack animation.
	 * 
	 * RESPONSIBILITY & DECOUPLING:
	 * Interfaces with the CombatStateMachine by writing to the Actor's intent/input buffer. 
	 * Reads from CombatBlackBoard for target tracking and alignment, keeping AI logic separate from combat physics.
	 * 
	 * STATE EVALUATION:
	 * - Success: Returns Success once the m_attackDuration timer completes.
	 * - Running: Returns Running while the attack timer is actively ticking (locks the tree into this node).
	 * - Failure: Returns Failure if the target is invalid or if the node is unexpectedly interrupted.
	 */
	class BTAttackNode : public BTNode
	{
	private:

		float m_attackDuration;
		float m_timer;

		float m_minAttackDis;
		bool m_isAttacking;

	public:

		BTAttackNode(float attackDuration, float minAttackDis = 12.0f);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
