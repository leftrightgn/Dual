#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"

namespace HEIN
{
	class BTCheckTetherNode : public BTNode
	{
	private:
		float m_maxTetherDistance;
	public:
		BTCheckTetherNode(float maxTetherDistance) : m_maxTetherDistance(maxTetherDistance) {}
		BTNodeState Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime) override;
	};
}
