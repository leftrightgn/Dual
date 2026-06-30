#include "pch.h"
#include "BTSelector.h"

void HEIN::BTSelector::AddChild(std::unique_ptr<HEIN::BTNode> child)
{
	m_children.push_back(std::move(child));
}

HEIN::BTNodeState HEIN::BTSelector::Tick(
	HEIN::Actor* self,
	HEIN::ActorManager* manager,
	HEIN::ActorID targetID,
	float deltaTime
)
{
	for (size_t i = 0; i < m_children.size(); ++i)
	{
		BTNodeState childState = m_children[i]->Tick(self, manager, targetID, deltaTime);

		if (childState != BTNodeState::Failure)
		{
			m_currentState = childState;

			return m_currentState;
		}
		m_currentState = BTNodeState::Failure;
		return m_currentState;
	}
}
