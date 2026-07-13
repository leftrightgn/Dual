#pragma once
#include <BehaviourTree/BTNode.h>


namespace HEIN
{
	class BTDodgeNode : public BTNode
	{
	private:

		float m_cooldown;
		float m_cooldownTimer;
		float m_dodgeStateTimer;
		float m_timeoutTimer;
		bool m_isDodging;
		bool m_hasDodgeStarted;

	public:

		BTDodgeNode(float cooldown = 3.0f);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
