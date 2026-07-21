#include "pch.h"
#include "BehaviourTreeComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include "../../External/Engine/Components/TransformComponent.h"

HEIN::BehaviourTreeComponent::BehaviourTreeComponent(Actor* owner)
	: IComponent(owner)
	, m_actorManager(nullptr)
	, m_targetID(INVALID_ACTOR_ID)
{
}

void HEIN::BehaviourTreeComponent::Initialize(
	std::unique_ptr<BTNode> root,
	HEIN::ActorManager* manager, 
	HEIN::ActorID targetID
)
{
	m_rootNode = std::move(root);
	m_actorManager = manager;
	m_targetID = targetID;
}

void HEIN::BehaviourTreeComponent::Update(float deltaTime)
{

	if (m_rootNode && m_actorManager && m_targetID != INVALID_ACTOR_ID)
	{
		HEIN::Actor* target = m_actorManager->GetActor(m_targetID);
		if (target == nullptr) return;
		HEIN::CombatBlackBoard* blackboard = GetOwner()->GetComponent<HEIN::CombatBlackBoard>();
		HEIN::TransformComponent* myTrans = GetOwner()->GetComponent<HEIN::TransformComponent>();
		HEIN::TransformComponent* targetTrans = target->GetComponent<HEIN::TransformComponent>();

		if (target && blackboard && myTrans && targetTrans)
		{
			DirectX::SimpleMath::Vector3 dir = targetTrans->GetPosition() - myTrans->GetPosition();
			dir.y = 0.0f;
			blackboard->distanceToTarget = dir.Length();

			if (blackboard->distanceToTarget > 0.001f) dir.Normalize();
			blackboard->dirToTarget = dir;
		}

		m_rootNode->Tick(GetOwner(), m_actorManager, m_targetID, deltaTime);
	}
}
