#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"


namespace HEIN
{
	/// <summary>
	/// Behavior Tree node for orbital movement around a target.
	/// Maintains a specific radius while moving laterally to reposition 
	/// and evade attacks, while keeping the target locked on.
	/// </summary>
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

		std::string GetNodeName() const override { return "Strafe Node"; }
		std::string GetNodeDetails() const override { return "Spd: " + std::to_string(m_strafeSpeed).substr(0, 4) + " | Dur: " + std::to_string(m_strafeDuration).substr(0, 4) + "s"; }
	};
}
