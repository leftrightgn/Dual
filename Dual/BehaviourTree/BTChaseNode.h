#pragma once
#include <BehaviourTree/BTNode.h>


namespace HEIN
{
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
