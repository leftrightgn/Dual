#include "pch.h"
#include "BTReturnToSpawnNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Components/TransformComponent.h"
#include "../../External/Engine/Message/Message.h"
#include "../../External/Engine/Message/Messenger.h"

namespace HEIN
{
BTReturnToSpawnNode::BTReturnToSpawnNode(float walkSpeed)
	: m_walkSpeed(walkSpeed)
{
}

void BTReturnToSpawnNode::OnEnter(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
{
	m_isActive = true;
	m_currentState = BTNodeState::Running;
	if (self)
	{
		auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
		if (blackboard)
		{
			blackboard->activeNodeName = GetNodeName();
			blackboard->isLockedOn = false;
			blackboard->isAttacking = false;
		}
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_EXIT_STRAFE);
	}
}

void BTReturnToSpawnNode::OnExit(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
{
	if (self)
	{
		auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
		if (blackboard)
		{
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
		}
	}
	m_isActive = false;
}

void BTReturnToSpawnNode::Abort(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID)
{
	OnExit(self, manager, targetID);
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

void BTReturnToSpawnNode::Reset()
{
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

BTNodeState BTReturnToSpawnNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float /*deltaTime*/)
{
	if (!self) return BTNodeState::Failure;

	auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	auto* transform = self->GetComponent<HEIN::TransformComponent>();

	if (!blackboard || !transform) return BTNodeState::Failure;

	if (!m_isActive || m_currentState != BTNodeState::Running)
	{
		OnEnter(self, manager, targetID);
	}

	DirectX::SimpleMath::Vector3 currentPos = transform->GetPosition();
	DirectX::SimpleMath::Vector3 dirToHome = blackboard->spawnPosition - currentPos;
	dirToHome.y = 0.0f;
	float distanceToHome = dirToHome.Length();

	if (distanceToHome < 2.0f)
	{
		blackboard->activeNodeName = "Idle at Spawn";
		blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
		blackboard->isLockedOn = false;
		blackboard->isLeashing = false;

		OnExit(self, manager, targetID);
		m_currentState = BTNodeState::Success;
		return BTNodeState::Success;
	}

	blackboard->activeNodeName = "Returning to Spawn";
	blackboard->isLockedOn = false;
	blackboard->isAttacking = false;

	dirToHome.Normalize();
	blackboard->moveIntent = dirToHome;
	blackboard->currentSpeed = m_walkSpeed;

	m_currentState = BTNodeState::Running;
	return BTNodeState::Running;
}
} // namespace HEIN
