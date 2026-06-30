#include "pch.h"
#include "BTStrafeNode.h"
#include <Components/TransformComponent.h>
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
	, m_strafeRight(goRight)
{
}

HEIN::BTNodeState HEIN::BTStrafeNode::Tick(HEIN::Actor* self, HEIN::ActorManager* manager, HEIN::ActorID targetID, float deltaTime)
{
	HEIN::CombatBlackBoard* bb = self->GetComponent<HEIN::CombatBlackBoard>();

	m_timer += deltaTime;
	if (m_timer >= m_strafeDuration)
	{
		bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
		m_timer = 0;
		return BTNodeState::Success;
	}

	DirectX::SimpleMath::Vector3 upVector = DirectX::SimpleMath::Vector3::Up;
	DirectX::SimpleMath::Vector3 strafeDir = m_strafeRight ?
		bb->dirToTarget.Cross(upVector) : bb->dirToTarget.Cross(bb->dirToTarget);

	strafeDir.Normalize();

	float distanceError = bb->distanceToTarget - m_orbitRadius;
	DirectX::SimpleMath::Vector3 correctionDir = bb->dirToTarget * (distanceError * 2.0f);

	bb->moveIntent = strafeDir + correctionDir;
	bb->moveIntent.y = 0.0f;
	bb->moveIntent.Normalize();
	bb->currentSpeed = m_strafeSpeed;

	return BTNodeState::Running;
}
