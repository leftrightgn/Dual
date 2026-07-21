#pragma once
#include "../../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"


namespace HEIN
{
	class BTStrafeNode : public BTNode
	{
	private:

		float m_strafeSpeed;
		float m_orbitRadius;
		float m_strafeDuration;
		float m_timer;
		bool m_strafeRight;

	public:

		BTStrafeNode(float speed, float radius, float duration, bool goRight);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
