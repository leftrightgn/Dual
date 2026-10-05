#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"
#include "BlackBoard/CombatBlackBoard.h"

namespace HEIN
{
	class BTCheckTargetStanceNode : public BTNode
	{
	private:

		CombatStance m_desiredStance;
		bool m_checkIsAttacking;

	public:

		BTCheckTargetStanceNode(CombatStance desiredStance);
		BTCheckTargetStanceNode(bool checkIsAttacking);
		BTNodeState Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime) override;
	};
}
