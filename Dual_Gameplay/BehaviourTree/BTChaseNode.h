#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"

namespace HEIN
{
	/// <summary>
	/// Behavior Tree node responsible for chasing a target.
	/// Moves the actor towards the target until it enters a specified attack range,
	/// clearing lock-on and strafe states while active.
	/// </summary>
	class BTChaseNode : public BTNode
	{
	private:

		float m_attackRange;
		float m_walkSpeed;
		bool m_succeedOnArrival;

	public:

		BTChaseNode(float attackRange, float walkSpeed, bool succeedOnArrival = false);

		void OnEnter(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID) override;
		void OnExit(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID) override;
		void Abort(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID) override;
		void Reset() override;

		BTNodeState Tick(
			HEIN::Actor* self,
			HEIN::ActorManager* manager,
			HEIN::ActorID targetID,
			float deltaTime
		) override;

		std::string GetNodeName() const override { return "Chase Node"; }
		std::string GetNodeDetails() const override { return "Spd: " + std::to_string(m_walkSpeed).substr(0, 4) + " | Range: " + std::to_string(m_attackRange).substr(0, 4); }
	};
}
