#pragma once
#include "../../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"

namespace HEIN
{
	/// <summary>
	/// Behavior Tree node responsible for chasing a target.
	/// Moves the actor towards the target until it enters a specified attack range,
	/// clearing lock-on and strafe states while active.
	/// </summary>
	class BTChaseNode : public BTNode
	{
	private:

		float m_attackRange;
		float m_walkSpeed;

	public:

		BTChaseNode(float attackRange, float walkSpeed);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
