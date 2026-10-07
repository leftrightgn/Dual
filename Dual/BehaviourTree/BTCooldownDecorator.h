#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"
#include <memory>

namespace HEIN
{
	class BTCooldownDecorator : public BTNode
	{
	private:

		float m_cooldownTime;
		float m_timer;
		std::unique_ptr<BTNode> m_child;

	public:

		BTCooldownDecorator(float cooldownTime, std::unique_ptr<BTNode> child);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
