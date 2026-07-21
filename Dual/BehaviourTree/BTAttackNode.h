#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"

namespace HEIN
{
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
