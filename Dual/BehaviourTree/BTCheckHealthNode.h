#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"

namespace HEIN
{
	class BTCheckHealthNode : public BTNode
	{
	private:

		float m_minPct;
		float m_maxPct;

	public:

		BTCheckHealthNode(float minPct, float maxPct);
		BTNodeState Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime) override;
	};
}
