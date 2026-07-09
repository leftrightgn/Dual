#include "pch.h"
#include "BTAttackNode.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Message/Messenger.h> 
#include <Message/Message.h>  
#include <cmath>

HEIN::BTAttackNode::BTAttackNode(
	float attackDuration
)
	: m_attackDuration(attackDuration)
	, m_timer(0.0f)
{
}

HEIN::BTNodeState HEIN::BTAttackNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;

	// Send attack inputs periodically so the FSM can buffer them for combos
	if (std::fmod(m_timer, 0.5f) < deltaTime)
	{
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_ACTION_ATTACK);
	}

	m_timer += deltaTime;

	if (m_timer >= m_attackDuration)
	{
		m_timer = 0;
		return BTNodeState::Success;
	}


	return BTNodeState::Running;
}