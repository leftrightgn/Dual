#pragma once
#include "../../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"

namespace HEIN
{
	/**
	 * @brief BTCheckDistance is a Condition Node that evaluates positional viability for a branch.
	 * 
	 * RESPONSIBILITY & DECOUPLING:
	 * Often placed as the first node in a BTSequence to act as a distance-based guard clause.
	 * Reads `distanceToTarget` directly from the CombatBlackBoard, meaning it does not calculate
	 * physics distance itself, but relies on TargetTrackingComponent's prior frame calculations.
	 * 
	 * STATE EVALUATION:
	 * - Success: Returns Success if CombatBlackBoard::distanceToTarget is between m_minDistance and m_maxDistance.
	 * - Failure: Returns Failure if the distance falls outside the bounds.
	 * - Running: Never returns Running (Condition nodes evaluate instantly).
	 */
	class BTCheckDistance : public BTNode
	{
	private:

		// @brief The minimum distance to the target required for this node to succeed.
		// PURPOSE: Defines the lower bound of the acceptable positional range, ensuring the actor isn't too close.
		float m_minDistance;

		// @brief The maximum distance to the target required for this node to succeed.
		// PURPOSE: Defines the upper bound of the acceptable positional range, ensuring the actor isn't too far.
		float m_maxDistance;

	public:

		BTCheckDistance(float minDistance, float maxDistance);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
