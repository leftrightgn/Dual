#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"

namespace HEIN
{
	class BTCheckHealth : public BTNode
	{
	private:

		float m_minPct;
		float m_maxPct;

	public:

		BTCheckHealth(float minPct, float maxPct);
		BTNodeState Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime) override;

		std::string GetNodeName() const override { return "Check Health"; }
		std::string GetNodeDetails() const override { return "Health: " + std::to_string(static_cast<int>(m_minPct * 100)) + "% - " + std::to_string(static_cast<int>(m_maxPct * 100)) + "%"; }
	};
}
