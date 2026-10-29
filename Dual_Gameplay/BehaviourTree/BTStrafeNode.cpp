#include "pch.h"
#include "BTStrafeNode.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "../../External/Engine/Message/Message.h"
#include "../../External/Engine/Message/Messenger.h"

namespace HEIN
{
BTStrafeNode::BTStrafeNode(float speed, float radius, float duration, bool goRight)
	: m_strafeSpeed(speed)
	, m_orbitRadius(radius)
	, m_strafeDuration(duration)
	, m_timer(0.0f)
	, m_strafeRight(goRight)
{
}

void BTStrafeNode::OnEnter(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
{
	m_timer = 0.0f;
	m_isActive = true;
	m_currentState = BTNodeState::Running;

	if (self)
	{
		auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
		if (blackboard)
		{
			blackboard->activeNodeName = GetNodeName();
			blackboard->isLockedOn = true;
		}
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_ENTER_STRAFE);
	}
}

void BTStrafeNode::OnExit(HEIN::Actor* self, HEIN::ActorManager* /*manager*/, HEIN::ActorID /*targetID*/)
{
	if (self)
	{
		auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
		if (blackboard)
		{
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
		}
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_EXIT_STRAFE);
		m_strafeRight = !m_strafeRight;
	}
	m_timer = 0.0f;
	m_isActive = false;
}

void BTStrafeNode::Abort(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID)
{
	OnExit(self, manager, targetID);
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

void BTStrafeNode::Reset()
{
	m_timer = 0.0f;
	m_isActive = false;
	m_currentState = BTNodeState::Failure;
}

BTNodeState BTStrafeNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	if (!self) return BTNodeState::Failure;

	auto* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (!blackboard) return BTNodeState::Failure;

	if (!m_isActive || m_currentState != BTNodeState::Running)
	{
		OnEnter(self, manager, targetID);
	}

	blackboard->activeNodeName = "Strafe Node";
	blackboard->isLockedOn = true;

	m_timer += deltaTime;

	if (m_timer >= m_strafeDuration)
	{
		OnExit(self, manager, targetID);
		m_currentState = BTNodeState::Success;
		return BTNodeState::Success;
	}

	DirectX::SimpleMath::Vector3 upVector = DirectX::SimpleMath::Vector3::Up;
	DirectX::SimpleMath::Vector3 strafeDir = m_strafeRight
		? blackboard->dirToTarget.Cross(upVector)
		: upVector.Cross(blackboard->dirToTarget);

	strafeDir.Normalize();

	float distanceError = blackboard->distanceToTarget - m_orbitRadius;
	DirectX::SimpleMath::Vector3 correctionDir = blackboard->dirToTarget * (distanceError * 2.0f);

	DirectX::SimpleMath::Vector3 finalMove = strafeDir + correctionDir;
	finalMove.y = 0.0f;
	if (finalMove.LengthSquared() > 0.001f)
	{
		finalMove.Normalize();
	}

	blackboard->moveIntent = finalMove;
	blackboard->currentSpeed = m_strafeSpeed;
	blackboard->localMoveIntent.x = m_strafeRight ? 1.0f : -1.0f;
	blackboard->localMoveIntent.z = 0.0f;

	m_currentState = BTNodeState::Running;
	return BTNodeState::Running;
}
} // namespace HEIN
