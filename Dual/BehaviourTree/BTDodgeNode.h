#pragma once
#include "../../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"


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
