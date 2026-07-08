#include "pch.h"
#include "BTStrafeNode.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Message/Messenger.h> 
#include <Message/Message.h>  

HEIN::BTStrafeNode::BTStrafeNode(
	float speed,
	float radius,
	float duration,
	bool goRight
)
	: m_strafeSpeed(speed)
	, m_orbitRadius(radius)
	, m_strafeDuration(duration)
	, m_timer(0.0f)
	, m_strafeRight(goRight)
{
}

HEIN::BTNodeState HEIN::BTStrafeNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	HEIN::CombatBlackBoard* blackboard = self->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard == nullptr) return BTNodeState::Failure;

	blackboard->isLockedOn = true;

	if (m_timer == 0.0f)
	{
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_ENTER_STRAFE);
	}

	m_timer += deltaTime;

	if (m_timer >= m_strafeDuration)
	{
		blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		Messenger::GetInstance()->Notify(self->GetID(), Message::PLAYER_EXIT_STRAFE);

		m_timer = 0;
		return BTNodeState::Success;
	}

	DirectX::SimpleMath::Vector3 upVector = DirectX::SimpleMath::Vector3::Up;
	DirectX::SimpleMath::Vector3 strafeDir = m_strafeRight ?
		blackboard->dirToTarget.Cross(upVector) : upVector.Cross(blackboard->dirToTarget);

	strafeDir.Normalize();

	float distanceError = blackboard->distanceToTarget - m_orbitRadius;
	DirectX::SimpleMath::Vector3 correctionDir = blackboard->dirToTarget * (distanceError * 2.0f);

	blackboard->moveIntent = strafeDir + correctionDir;
	blackboard->moveIntent.y = 0.0f;
	blackboard->moveIntent.Normalize();
	blackboard->currentSpeed = m_strafeSpeed;


	blackboard->localMoveIntent.x = m_strafeRight ? -1.0f : 1.0f;

	return BTNodeState::Running;
}