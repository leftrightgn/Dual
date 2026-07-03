#include "pch.h"
#include "BTStrafeNode.h"
#include <BlackBoard/CombatBlackBoard.h>

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

	m_timer += deltaTime;
	if (m_timer >= m_strafeDuration)
	{
		blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		blackboard->isStrafingIntent = false;
		m_timer = 0;
		return BTNodeState::Success;
	}

	blackboard->isStrafingIntent = true;

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

	return BTNodeState::Running;
}
