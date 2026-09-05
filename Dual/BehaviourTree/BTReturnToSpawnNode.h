#pragma once
#include "BehaviourTree/BTNode.h"
#include "Entities/Actor.h"
#include "Entities/ActorManager.h"

namespace HEIN
{
	/**
	 * @brief BTReturnToSpawnNode handles "Leashing".
	 *
	 * RESPONSIBILITY:
	 * If the player runs away, this node directs the AI back to its original spawn point.
	 * Returns Failure if the AI is already home, allowing the tree to fall back to Idle.
	 */
	class BTReturnToSpawnNode : public BTNode
	{
	private:
		float m_walkSpeed;

	public:
		BTReturnToSpawnNode(float walkSpeed = 20.0f);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}