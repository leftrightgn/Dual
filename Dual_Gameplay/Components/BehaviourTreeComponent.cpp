#include "pch.h"
#include <functional>
#include "../Factory/ActorFactory.h"
#include "BehaviourTreeComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "CombatStateMachineComponent.h"
#include "../../External/Engine/Components/TransformComponent.h"
#if HEIN_WITH_EDITOR
#include "../../External/Engine/ImGui/imgui.h"
#endif

namespace HEIN
{
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

void HEIN::BehaviourTreeComponent::EnsureInitialized()
{
	if (!m_rootNode && m_owner)
	{
		HEIN::ActorFactory::SetupEnemyAI(m_owner, m_actorManager, m_targetID);
	}
}

void HEIN::BehaviourTreeComponent::Start()
{
	auto* blackboard = GetOwner()->GetComponent<HEIN::CombatBlackBoard>();
	if (blackboard)
	{
		if (!m_hasSetSpawnPosition && blackboard->hasSetSpawnPosition)
		{
			m_spawnPosition = blackboard->spawnPosition;
			m_hasSetSpawnPosition = true;
		}
		else if (m_hasSetSpawnPosition)
		{
			blackboard->spawnPosition = m_spawnPosition;
			blackboard->hasSetSpawnPosition = true;
		}
	}
}

void HEIN::BehaviourTreeComponent::Update(float deltaTime)
{
	auto* blackboard = GetOwner()->GetComponent<HEIN::CombatBlackBoard>();

	// Handle Paused / Cutscene State
	if (m_isPaused)
	{
		if (m_rootNode && (m_rootNode->IsActive() || m_rootNode->IsRunning()))
		{
			m_rootNode->Abort(GetOwner(), m_actorManager, m_targetID);
		}

		if (blackboard)
		{
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->lookIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->isLockedOn = false;
			blackboard->isAttacking = false;
			blackboard->activeNodeName = "Cinematic (Paused)";
		}
		return;
	}

	// Handle Target Validation
	HEIN::Actor* target = (m_actorManager && m_targetID != INVALID_ACTOR_ID) ? m_actorManager->GetActor(m_targetID) : nullptr;
	bool isTargetInvalid = (target == nullptr || m_targetID == INVALID_ACTOR_ID || !target->IsActive() || target->GetActorType() == HEIN::ActorType::Ghost);

	if (isTargetInvalid)
	{
		if (m_rootNode && (m_rootNode->IsActive() || m_rootNode->IsRunning()))
		{
			m_rootNode->Abort(GetOwner(), m_actorManager, m_targetID);
		}

		if (blackboard)
		{
			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->lookIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->isLockedOn = false;
			blackboard->isAttacking = false;
			blackboard->activeNodeName = (target && target->GetActorType() == HEIN::ActorType::Ghost)
				? "Idle (Ignoring Ghost)"
				: "Idle (Target Lost)";
		}
		if (auto* sm = GetOwner()->GetComponent<HEIN::CombatStateMachineComponent>())
		{
			if (sm->GetCurrentStateName() != "Idle")
			{
				sm->ChangeState("Idle", 0.2f);
			}
		}
		return;
	}

	// Handle Owner Incapacitation / Hit Stun / Poise Break
	if (blackboard)
	{
		bool isOwnerIncapacitated = (blackboard->currentStance == CombatStance::Staggered || blackboard->currentHealth <= 0.0f);
		if (isOwnerIncapacitated)
		{
			if (m_rootNode && (m_rootNode->IsActive() || m_rootNode->IsRunning()))
			{
				m_rootNode->Abort(GetOwner(), m_actorManager, m_targetID);
			}

			blackboard->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->lookIntent = DirectX::SimpleMath::Vector3::Zero;
			blackboard->isAttacking = false;
			blackboard->activeNodeName = (blackboard->currentHealth <= 0.0f)
				? "Incapacitated (Dead)"
				: "Incapacitated (Staggered)";
			return;
		}
	}

	// Execute Active Tree
	if (m_rootNode && m_actorManager)
	{
		HEIN::TransformComponent* myTrans = GetOwner()->GetComponent<HEIN::TransformComponent>();
		HEIN::TransformComponent* targetTrans = target->GetComponent<HEIN::TransformComponent>();

		if (blackboard && myTrans && targetTrans)
		{
			DirectX::SimpleMath::Vector3 dir = targetTrans->GetPosition() - myTrans->GetPosition();
			dir.y = 0.0f;
			blackboard->distanceToTarget = dir.Length();

			if (blackboard->distanceToTarget > 0.001f) dir.Normalize();
			blackboard->dirToTarget = dir;

			if (auto* targetBB = target->GetComponent<HEIN::CombatBlackBoard>())
			{
				blackboard->targetStance = targetBB->currentStance;
				blackboard->targetHealth = targetBB->currentHealth;
				blackboard->isTargetAttacking = (targetBB->currentStance == CombatStance::OneHand ||
					targetBB->currentStance == CombatStance::AttackWindup ||
					targetBB->currentStance == CombatStance::AttackRelese ||
					targetBB->currentStance == CombatStance::AttackRecovery);
			}
		}

		if (blackboard)
		{
			blackboard->elapsedCombatTime += deltaTime;
			if (blackboard->actionCooldownTimer > 0.0f)
			{
				blackboard->actionCooldownTimer -= deltaTime;
			}
		}

		m_rootNode->Tick(GetOwner(), m_actorManager, m_targetID, deltaTime);

		if (blackboard)
		{
			blackboard->hasSetSpawnPosition = m_hasSetSpawnPosition;
			blackboard->spawnPosition = m_spawnPosition;
			blackboard->aggroRange = m_aggroRange;
		}
	}
}

void HEIN::BehaviourTreeComponent::LateUpdate(float /*deltaTime*/)
{
#if HEIN_WITH_EDITOR
	if (m_showNodeEditor)
	{
		RenderNodeGraph();
	}
#endif
}

#if HEIN_WITH_EDITOR
void HEIN::BehaviourTreeComponent::OnInspectorGUI(GameContext& /*gamecontext*/)
{
	if (ImGui::CollapsingHeader("BehaviourTree Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Pause Behaviour Tree", &m_isPaused);
		ImGui::Checkbox("Open Visual Behaviour Tree View", &m_showNodeEditor);
		ImGui::Separator();
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
#else
void HEIN::BehaviourTreeComponent::OnInspectorGUI(GameContext& /*gamecontext*/) {}
#endif

#if HEIN_WITH_EDITOR
void HEIN::BehaviourTreeComponent::RenderNodeGraph()
{
	EnsureInitialized();
	if (!m_rootNode)
	{
		ImGui::SetNextWindowSize(ImVec2(850, 600), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Behaviour Tree - Visual Node View", &m_showNodeEditor))
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No Behaviour Tree initialized on this actor.");
			if (ImGui::Button("Initialize Default AI Tree"))
			{
				EnsureInitialized();
			}
			ImGui::End();
		}
		return;
	}

	GenericGraphDesc graph;
	graph.graphTitle = "Behaviour Tree - Visual Node View";

	HEIN::CombatBlackBoard* blackboard = m_owner ? m_owner->GetComponent<HEIN::CombatBlackBoard>() : nullptr;
	std::string activeName = blackboard ? blackboard->activeNodeName : "";
	graph.activeNodeName = activeName;

	int nextNodeId = 1;
	int nextPinId = 1000;
	int nextLinkId = 5000;

	std::function<int(const BTNode*, int)> traverseNode = [&](const BTNode* node, int parentOutputPinId) -> int
	{
		if (!node) return 0;

		int currentId = nextNodeId++;
		GenericGraphNode gNode;
		gNode.id = currentId;
		gNode.title = node->GetNodeName();
		gNode.subtitle = node->GetNodeDetails();

		// Highlight currently running node
		bool isRunning = (node->GetState() == BTNodeState::Running) ||
		                 (!activeName.empty() && activeName.find(gNode.title) != std::string::npos);
		gNode.isActive = isRunning;

		// Input pin (connects to parent)
		int inputPinId = nextPinId++;
		gNode.inputPins.push_back({ inputPinId, "In", true });

		if (parentOutputPinId > 0)
		{
			GenericGraphLink link;
			link.id = nextLinkId++;
			link.startPinId = parentOutputPinId;
			link.endPinId = inputPinId;
			link.isPulsing = isRunning;
			link.pulseTimer = 0.5f;
			graph.links.push_back(link);
		}

		// Child output pins
		size_t childCount = node->GetChildCount();
		for (size_t i = 0; i < childCount; ++i)
		{
			int outputPinId = nextPinId++;
			char pinLabel[32];
			sprintf_s(pinLabel, "[%zu]", i + 1);
			gNode.outputPins.push_back({ outputPinId, pinLabel, false });

			const BTNode* child = node->GetChild(i);
			traverseNode(child, outputPinId);
		}

		graph.nodes.push_back(gNode);
		return currentId;
	};

	traverseNode(m_rootNode.get(), 0);

	int selectedNodeId = 0;
	m_nodeGraphView.Render(graph, &m_showNodeEditor, &selectedNodeId);
}
#else
void HEIN::BehaviourTreeComponent::RenderNodeGraph() {}
#endif

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
} // namespace HEIN
