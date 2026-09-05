#include "pch.h"
#include "BehaviourTreeComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include "../../External/Engine/Components/TransformComponent.h"
#include <ImGui/imgui.h>

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

void HEIN::BehaviourTreeComponent::Start()
{
	HEIN::CombatBlackBoard* blackboard = GetOwner()->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard)
	{
		if (!m_hasSetSpawnPosition && blackboard->hasSetSpawnPosition)
		{
			m_spawnPosition = blackboard->spawnPosition;
			m_hasSetSpawnPosition = true;
		}
	}
}

void HEIN::BehaviourTreeComponent::Update(float deltaTime)
{
	// Execute AI logic only if the tree, manager, and a valid target exist
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

		// Synchronize configurable parameters to the Blackboard for BT nodes to read
		if (blackboard)
		{
			blackboard->hasSetSpawnPosition = m_hasSetSpawnPosition;
			blackboard->spawnPosition = m_spawnPosition;
			blackboard->aggroRange = m_aggroRange;
		}
		
	}
}

void HEIN::BehaviourTreeComponent::OnInspectorGUI(GameContext& /*gamecontext*/)
{
	if (ImGui::CollapsingHeader("BehaviourTree Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		HEIN::CombatBlackBoard* blackboard = m_owner ? m_owner->GetComponent<HEIN::CombatBlackBoard>() : nullptr;
		if (blackboard)
		{
			ImGui::Text("Active Node: %s", blackboard->activeNodeName.c_str());
			ImGui::Text("Distance To Target: %.2f", blackboard->distanceToTarget);
			ImGui::Text("Is Locked On: %s", blackboard->isLockedOn ? "Yes" : "No");
			ImGui::Text("Current Health: %.2f / 100", blackboard->currentHealth);
			ImGui::Text("Stamina: %.2f / %.2f", blackboard->currentStamina, blackboard->maxStamina);
			ImGui::Separator();
		}

		ImGui::Text("Spawn/Tether Point");
		ImGui::DragFloat3("Spawn Position", &m_spawnPosition.x, 0.1f);
		ImGui::DragFloat("Aggro/Tether Range", &m_aggroRange, 0.5f, 0.0f, 300.0f);
		if (ImGui::Button("Set Spawn To Current Position"))
		{
			if (m_owner)
			{
				if (auto* transform = m_owner->GetComponent<TransformComponent>())
				{
					m_spawnPosition = transform->GetPosition();
					m_hasSetSpawnPosition = true;
				}
			}
		}
	}
}

nlohmann::json HEIN::BehaviourTreeComponent::Serialize()
{
	nlohmann::json data;
	data["spawnPosition"] = nlohmann::json::array({ m_spawnPosition.x, m_spawnPosition.y, m_spawnPosition.z });
	data["aggroRange"] = m_aggroRange;
	data["hasSetSpawnPosition"] = m_hasSetSpawnPosition;
	return data;
}

void HEIN::BehaviourTreeComponent::Deserialize(const nlohmann::json& data)
{
	if (data.contains("spawnPosition") && data["spawnPosition"].is_array() && data["spawnPosition"].size() == 3)
	{
		m_spawnPosition = DirectX::SimpleMath::Vector3(data["spawnPosition"][0], data["spawnPosition"][1], data["spawnPosition"][2]);
	}
	if (data.contains("aggroRange"))
	{
		m_aggroRange = data["aggroRange"];
	}
	if (data.contains("hasSetSpawnPosition"))
	{
		m_hasSetSpawnPosition = data["hasSetSpawnPosition"];
	}
}

