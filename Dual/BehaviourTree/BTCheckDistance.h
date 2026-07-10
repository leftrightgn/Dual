#pragma once
#include <BehaviourTree/BTNode.h>

namespace HEIN
{
	class BTCheckDistance : public BTNode
	{
	private:

		float m_minDistance;
		float m_maxDistance;

	public:

		BTCheckDistance(float minDistance, float maxDistance);

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;
	};
}
