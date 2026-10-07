#include "pch.h"
#include "BTCooldownDecorator.h"


HEIN::BTCooldownDecorator::BTCooldownDecorator(float cooldownTime, std::unique_ptr<BTNode> child)
	: m_cooldownTime(cooldownTime)
	, m_child(std::move(child))
{
}

HEIN::BTNodeState HEIN::BTCooldownDecorator::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	if (m_timer > 0.0f)
	{
		m_timer -= deltaTime;
		return BTNodeState::Failure;
	}

	if (!m_child) return BTNodeState::Failure;

	BTNodeState state = m_child->Tick(self, manager, targetID, deltaTime);
	if (state == BTNodeState::Success)
	{
		m_timer = m_cooldownTime;
	}
	return state;
}
