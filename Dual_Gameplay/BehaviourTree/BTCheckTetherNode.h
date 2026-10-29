#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"

namespace HEIN
{
	class BTCheckTetherNode : public BTNode
	{
	private:
		float m_maxTetherDistance;
	public:
		BTCheckTetherNode(float maxTetherDistance);
		BTNodeState Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime) override;

		std::string GetNodeName() const override { return "Check Tether"; }
		std::string GetNodeDetails() const override { return "Leash Range: " + std::to_string(m_maxTetherDistance).substr(0, 4); }
	};
}
