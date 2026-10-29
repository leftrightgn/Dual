#pragma once
#include "BlackBoard/CombatBlackBoard.h"
#include "../../External/Engine/BehaviourTree/BTNode.h"

namespace HEIN
{
	class BTCheckActionCooldown : public BTNode
	{
	public:
		BTCheckActionCooldown() = default;

		BTNodeState Tick(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/, float /*deltaTime*/) override
		{
			if (!self) return BTNodeState::Failure;
			auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
			if (!blackboard) return BTNodeState::Failure;

			if (blackboard->actionCooldownTimer <= 0.0f)
			{
				return BTNodeState::Success;
			}

			return BTNodeState::Failure;
		}

		std::string GetNodeName() const override { return "Check Action Cooldown"; }
		std::string GetNodeDetails() const override { return "Ready when <= 0.0s"; }
	};
}
