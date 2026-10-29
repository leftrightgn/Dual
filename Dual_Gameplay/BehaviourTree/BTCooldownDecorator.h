#pragma once
#include "../../External/Engine/BehaviourTree/BTNode.h"
#include <memory>
#include <chrono>

namespace HEIN
{
	class BTCooldownDecorator : public BTNode
	{
	private:

		float m_cooldownTime = 0.0f;
		float m_cooldownExpirationTime = 0.0f;
		float m_fallbackTimer = 0.0f;
		std::unique_ptr<BTNode> m_child;

	public:

		BTCooldownDecorator(float cooldownTime, std::unique_ptr<BTNode> child);

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

		std::string GetNodeName() const override { return "Cooldown Decorator"; }
		std::string GetNodeDetails() const override { return "Cooldown: " + std::to_string(m_cooldownTime).substr(0, 4) + "s"; }
		size_t GetChildCount() const override { return m_child ? 1 : 0; }
		const BTNode* GetChild(size_t index) const override { return (index == 0) ? m_child.get() : nullptr; }
		BTNode* GetChildMutable() { return m_child.get(); }

		void ResetCooldown()
		{
			m_cooldownExpirationTime = 0.0f;
			m_fallbackTimer = 0.0f;
		}
	};
}
