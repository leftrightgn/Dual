#pragma once
#include <BehaviourTree/BTNode.h>


namespace HEIN
{
	class BTAttackNode : public BTNode
	{
	private:

		float m_attackDuration;
		float m_timer;

	public:

		BTAttackNode(float attackDuration);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
