#pragma once
#include "BehaviourTree/BTNode.h"
#include "Entities/Actor.h"
#include "Entities/ActorManager.h"

namespace HEIN
{
	/**
	 * @brief BTIdleNode is a fallback Action Node.
	 *
	 * RESPONSIBILITY:
	 * Clears the actor's movement intent so they stand perfectly still when
	 * there are no valid actions (e.g., the player is outside their aggro zone).
	 */
	class BTIdleNode : public BTNode
	{
	public:
		BTIdleNode() = default;

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}