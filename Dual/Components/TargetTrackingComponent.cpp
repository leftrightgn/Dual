#include "pch.h"
#include "TargetTrackingComponent.h"
#include <Entities/ActorManager.h>
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/TransformComponent.h>

HEIN::ActorID HEIN::TargetTrackingComponent::FindBestTarget() const
{
	if (m_actorManager == nullptr || m_transform == nullptr) return HEIN::INVALID_ACTOR_ID;

	DirectX::SimpleMath::Vector3 myPos = m_transform->GetPosition();
	HEIN::ActorID closestTargetID = HEIN::INVALID_ACTOR_ID;
	float minDistanceSq = FLT_MAX;

	for (auto& actorPair : m_actorManager->GetAllActors())
	{
		HEIN::Actor* actor = actorPair.second.get();

		if (actor == nullptr || actor == m_owner) continue;

		if (actor->GetActorType() == m_targetTypeToFind)
		{
			HEIN::TransformComponent* targetTrans = actor->GetComponent<HEIN::TransformComponent>();

			if (targetTrans != nullptr)
			{
				DirectX::SimpleMath::Vector3 targetPos = targetTrans->GetPosition();

				DirectX::SimpleMath::Vector3 dist = targetPos - myPos;
				float distSq = dist.LengthSquared();

				if (distSq < minDistanceSq)
				{
					minDistanceSq = distSq;
					closestTargetID = actor->GetID();
				}
			}
			
		}
	}

	return closestTargetID;
}

HEIN::TargetTrackingComponent::TargetTrackingComponent(
	Actor* owner,
	ActorManager* manager,
	HEIN::ActorType targetType
)
	: IComponent(owner)
	, m_actorManager(manager)
	, m_targetTypeToFind(targetType)
{
}

void HEIN::TargetTrackingComponent::Start()
{
	m_blackboard = m_owner->GetComponent<HEIN::CombatBlackBoard>();
	m_transform = m_owner->GetComponent<HEIN::TransformComponent>();
}

void HEIN::TargetTrackingComponent::Update(float deltaTime)
{
	if (!m_blackboard || !m_transform || !m_actorManager) return;

	if (m_blackboard->lockOnIntent && m_blackboard->lockedTargetID == HEIN::INVALID_ACTOR_ID)
	{
		m_blackboard->lockedTargetID = FindBestTarget();
	}
	else if (!m_blackboard->lockOnIntent)
	{
		m_blackboard->lockedTargetID = HEIN::INVALID_ACTOR_ID;
		m_blackboard->dirToTarget = DirectX::SimpleMath::Vector3::Zero;
		m_blackboard->distanceToTarget = 0.0f;
		m_blackboard->isLockedOn = false;
	}

	if (m_blackboard->lockedTargetID != HEIN::INVALID_ACTOR_ID)
	{
		HEIN::Actor* target = m_actorManager->GetActor(m_blackboard->lockedTargetID);

		if (target != nullptr)
		{
			HEIN::TransformComponent* targetTrans = target->GetComponent<HEIN::TransformComponent>();
			
			if (targetTrans != nullptr)
			{
				DirectX::SimpleMath::Vector3 myPos = m_transform->GetPosition();
				DirectX::SimpleMath::Vector3 targetPos = targetTrans->GetPosition();

				DirectX::SimpleMath::Vector3 dir = targetPos - myPos;

				m_blackboard->distanceToTarget = dir.Length();
				dir.y = 0.0f;

				if (dir.LengthSquared() > 0.001f)
				{
					dir.Normalize();
					m_blackboard->dirToTarget = dir;
					m_blackboard->isLockedOn = true;
				}
			}
		}

	}
}
