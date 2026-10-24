#include "pch.h"
#include "EncounterTriggerComponent.h"
#include "BehaviourTreeComponent.h"
#include "CombatStateMachineComponent.h"
#include "CharacterMovementComponent.h"
#include "BlackBoard/BossBlackBoard.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "../../External/Engine/Camera/CameraController.h"
#include "../../External/Engine/Camera/CinematicMode.h"
#include "../../External/Engine/Camera/LockOnCameraMode.h"
#include "../../External/Engine/Components/RigidBodyComponent.h"
#include "../../External/Engine/Components/SkinnedModelComponent.h"
#include "../../External/Engine/Components/TransformComponent.h"
#include "../../External/Engine/Entities/ActorManager.h"
#include "../../External/Engine/Framework/GameContext.h"
#include "../../External/Engine/ImGui/imgui.h"

HEIN::EncounterTriggerComponent::EncounterTriggerComponent(Actor* owner)
	: IComponent(owner)
	, m_enabled(true)
	, m_hasTriggered(false)
	, m_drawDebugRange(true)
	, m_freezeOnTrigger(true)
	, m_triggerVictoryOnDeath(true)
	, m_triggerDistance(70.0f)
	, m_maxDuration(8.5f)
	, m_encounterTimer(0.0f)
	, m_isPlayingCutscene(false)
	, m_cinematicTrackName("Boss_Intro")
	, m_introAnimName("Idle")
{
}

void HEIN::EncounterTriggerComponent::Start()
{
	// - Hold owner AI paused at spawn if encounter has not yet triggered
	if (m_freezeOnTrigger && !m_hasTriggered && GetOwner())
	{
		if (auto* bt = GetOwner()->GetComponent<BehaviourTreeComponent>())
		{
			bt->SetPaused(true);
		}
	}
}

void HEIN::EncounterTriggerComponent::Update(float deltaTime)
{
	if (m_isPlayingCutscene)
	{
		m_encounterTimer += deltaTime;
	}
}

bool HEIN::EncounterTriggerComponent::CheckAndTrigger(GameContext& gameContext, const DirectX::SimpleMath::Vector3& playerPos)
{
	if (!m_enabled || m_hasTriggered || m_isPlayingCutscene)
	{
		return false;
	}

	if (!GetOwner())
	{
		return false;
	}

	if (gameContext.actorManager != nullptr)
	{
		if (auto* player = gameContext.actorManager->GetFirstActorByType(ActorType::Player))
		{
			if (player->GetActorType() == ActorType::Ghost)
			{
				return false;
			}
		}
		else if (gameContext.actorManager->GetFirstActorByType(ActorType::Ghost) != nullptr)
		{
			return false;
		}
	}

	auto* transform = GetOwner()->GetComponent<TransformComponent>();
	if (!transform)
	{
		return false;
	}

	// - Evaluate planar horizontal distance between player and encounter actor
	DirectX::SimpleMath::Vector3 diff = playerPos - transform->GetPosition();
	diff.y = 0.0f;
	float distSq = diff.LengthSquared();

	if (distSq <= m_triggerDistance * m_triggerDistance)
	{
		StartEncounter(gameContext);
		return true;
	}

	return false;
}

void HEIN::EncounterTriggerComponent::StartEncounter(GameContext& gameContext)
{
	m_hasTriggered = true;
	m_isPlayingCutscene = true;
	m_encounterTimer = 0.0f;

	if (m_freezeOnTrigger)
	{
		FreezeEncounterActors(gameContext);
	}

	// - Switch camera to cinematic mode and trigger track
	if (gameContext.mainCamera)
	{
		gameContext.mainCamera->RequestSwitch(CameraType::Cinematic);
		gameContext.mainCamera->ApplyRequest();

		if (auto* cinematic = dynamic_cast<CinematicMode*>(gameContext.mainCamera->GetCurrentCameraMode()))
		{
			cinematic->PlayTrack(m_cinematicTrackName);
		}
	}
}

void HEIN::EncounterTriggerComponent::EndEncounter(GameContext& gameContext)
{
	m_isPlayingCutscene = false;

	// - Hold boss AI paused while camera transitions back behind the player
	m_isWakingUp = true;
	m_wakeTimer = 0.0f;
	if (GetOwner())
	{
		if (auto* bt = GetOwner()->GetComponent<BehaviourTreeComponent>())
		{
			bt->SetPaused(true);
		}
	}

	// - Transition to LockOn camera focused on encounter actor, or fallback to spring mode
	if (gameContext.mainCamera)
	{
		bool isPlayerGhost = false;
		if (gameContext.actorManager != nullptr)
		{
			if (auto* player = gameContext.actorManager->GetFirstActorByType(ActorType::Player))
			{
				if (player->GetActorType() == ActorType::Ghost)
				{
					isPlayerGhost = true;
				}
			}
			else if (auto* ghost = gameContext.actorManager->GetFirstActorByType(ActorType::Ghost))
			{
				isPlayerGhost = true;
			}
		}

		if (m_enableLockOnOnEnd && GetOwner() && !isPlayerGhost)
		{
			gameContext.mainCamera->RequestSwitch(CameraType::LockOn);
			gameContext.mainCamera->ApplyRequest();

			if (auto* lockOn = dynamic_cast<LockOnCameraMode*>(gameContext.mainCamera->GetCurrentCameraMode()))
			{
				lockOn->SetTargetID(GetOwner()->GetID());
			}
		}
		else
		{
			gameContext.mainCamera->RequestSwitch(CameraType::Spring);
		}
		gameContext.mainCamera->UpdateMouseMode();
	}
}

void HEIN::EncounterTriggerComponent::SkipCutscene(GameContext& gameContext)
{
	if (m_isPlayingCutscene)
	{
		if (gameContext.mainCamera)
		{
			if (auto* cinematic = dynamic_cast<CinematicMode*>(gameContext.mainCamera->GetCurrentCameraMode()))
			{
				cinematic->Skip();
			}
		}

		EndEncounter(gameContext);
	}
}

void HEIN::EncounterTriggerComponent::UpdateEncounter(float deltaTime, GameContext& gameContext)
{
	if (m_isWakingUp)
	{
		m_wakeTimer += deltaTime;

		bool cameraBlending = false;
		if (gameContext.mainCamera != nullptr)
		{
			cameraBlending = gameContext.mainCamera->IsBlending();
		}

		// - Hold boss AI paused until camera blend completes and wake timer elapses
		if (!cameraBlending && m_wakeTimer >= m_aiWakeDelay)
		{
			m_isWakingUp = false;
			if (m_freezeOnTrigger)
			{
				UnfreezeEncounterActors(gameContext);
			}
		}
		else
		{
			if (GetOwner())
			{
				if (auto* bt = GetOwner()->GetComponent<BehaviourTreeComponent>())
				{
					bt->SetPaused(true);
				}
			}
		}
	}
}

void HEIN::EncounterTriggerComponent::ResetEncounter()
{
	m_hasTriggered = false;
	m_isPlayingCutscene = false;
	m_isWakingUp = false;
	m_wakeTimer = 0.0f;
	m_encounterTimer = 0.0f;

	if (m_freezeOnTrigger && GetOwner())
	{
		if (auto* bt = GetOwner()->GetComponent<BehaviourTreeComponent>())
		{
			bt->SetPaused(true);
		}
	}
}

void HEIN::EncounterTriggerComponent::FreezeEncounterActors(GameContext& gameContext)
{
	// - Freeze local encounter actor AI and motion
	if (GetOwner())
	{
		if (auto* bt = GetOwner()->GetComponent<BehaviourTreeComponent>())
		{
			bt->SetPaused(true);
		}

		if (auto* bb = GetOwner()->GetComponent<BossBlackBoard>())
		{
			bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
			bb->isLockedOn = false;
			bb->isAttacking = false;
			bb->activeNodeName = "Cinematic (Paused)";
		}

		if (auto* rb = GetOwner()->GetComponent<RigidBodyComponent>())
		{
			rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
		}

		if (auto* cm = GetOwner()->GetComponent<CharacterMovementComponent>())
		{
			cm->ClearImpulse();
		}

		if (auto* combat = GetOwner()->GetComponent<CombatStateMachineComponent>())
		{
			if (combat->GetCurrentStateName() != "Idle")
			{
				combat->ChangeState("Idle", 0.1f);
			}
		}

		if (auto* model = GetOwner()->GetComponent<SkinnedModelComponent>())
		{
			if (model->HasAnimation(m_introAnimName))
			{
				model->ChangeAnimation(m_introAnimName);
			}
		}
	}

	// - Freeze player movement, clear input intent, and hold idle stance
	if (gameContext.actorManager)
	{
		if (auto* player = gameContext.actorManager->GetFirstActorByType(ActorType::Player))
		{
			if (auto* bb = player->GetComponent<CombatBlackBoard>())
			{
				bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
				bb->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
				bb->currentVelocity = DirectX::SimpleMath::Vector3::Zero;
			}

			if (auto* rb = player->GetComponent<RigidBodyComponent>())
			{
				rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
			}

			if (auto* cm = player->GetComponent<CharacterMovementComponent>())
			{
				cm->ClearImpulse();
			}

			if (auto* combat = player->GetComponent<CombatStateMachineComponent>())
			{
				if (combat->GetCurrentStateName() != "Idle")
				{
					combat->ChangeState("Idle", 0.1f);
				}
			}

			if (auto* model = player->GetComponent<SkinnedModelComponent>())
			{
				if (model->HasAnimation("Idle"))
				{
					model->ChangeAnimation("Idle");
				}
			}
		}
	}
}

void HEIN::EncounterTriggerComponent::UnfreezeEncounterActors(GameContext& /*gameContext*/)
{
	// - Unpause local encounter actor AI
	if (GetOwner())
	{
		if (auto* bt = GetOwner()->GetComponent<BehaviourTreeComponent>())
		{
			bt->SetPaused(false);
		}

		if (auto* model = GetOwner()->GetComponent<SkinnedModelComponent>())
		{
			if (model->HasAnimation("Idle"))
			{
				model->ChangeAnimation("Idle");
			}
		}
	}
}

void HEIN::EncounterTriggerComponent::Draw(
	GameContext& gameContext,
	const DirectX::SimpleMath::Matrix& /*world*/,
	const DirectX::SimpleMath::Matrix& /*view*/,
	const DirectX::SimpleMath::Matrix& /*proj*/
)
{
	if (!m_drawDebugRange) return;
	if (gameContext.debugCollisionRenderer == nullptr) return;

	DirectX::SimpleMath::Vector3 center(0.0f, 0.0f, 0.0f);
	if (GetOwner())
	{
		if (auto* transform = GetOwner()->GetComponent<TransformComponent>())
		{
			center = transform->GetPosition();
		}
		else
		{
			return;
		}
	}
	else
	{
		return;
	}

	// - Draw Cyan 3D wireframe bounding sphere around encounter anchor
	DirectX::BoundingSphere sphere(center, m_triggerDistance);
	DirectX::XMVECTORF32 cyanColor = { 0.0f, 0.85f, 1.0f, 0.85f };
	gameContext.debugCollisionRenderer->QueueSphere(sphere, cyanColor);

	// - Draw Yellow ground-level circular ring on arena floor (64 segments)
	const int segments = 64;
	DirectX::XMVECTORF32 yellowColor = { 1.0f, 0.9f, 0.15f, 0.95f };
	float groundY = center.y + 0.15f;

	for (int i = 0; i < segments; ++i)
	{
		float angle1 = (DirectX::XM_2PI / segments) * static_cast<float>(i);
		float angle2 = (DirectX::XM_2PI / segments) * static_cast<float>(i + 1);

		DirectX::SimpleMath::Vector3 p1 = center + DirectX::SimpleMath::Vector3(cosf(angle1) * m_triggerDistance, 0.0f, sinf(angle1) * m_triggerDistance);
		DirectX::SimpleMath::Vector3 p2 = center + DirectX::SimpleMath::Vector3(cosf(angle2) * m_triggerDistance, 0.0f, sinf(angle2) * m_triggerDistance);
		p1.y = groundY;
		p2.y = groundY;

		gameContext.debugCollisionRenderer->QueueLine(p1, p2, yellowColor);
	}

	// - Draw secondary Lime-Green ground-level ring for Lock-On auto switch distance (64 segments)
	if (m_enableProximityLockOn && m_lockOnDistance > 0.0f)
	{
		DirectX::XMVECTORF32 lockOnRingColor = { 0.2f, 1.0f, 0.4f, 0.9f };
		float lockGroundY = center.y + 0.10f;
		for (int i = 0; i < segments; ++i)
		{
			float angle1 = (DirectX::XM_2PI / segments) * static_cast<float>(i);
			float angle2 = (DirectX::XM_2PI / segments) * static_cast<float>(i + 1);

			DirectX::SimpleMath::Vector3 p1 = center + DirectX::SimpleMath::Vector3(cosf(angle1) * m_lockOnDistance, 0.0f, sinf(angle1) * m_lockOnDistance);
			DirectX::SimpleMath::Vector3 p2 = center + DirectX::SimpleMath::Vector3(cosf(angle2) * m_lockOnDistance, 0.0f, sinf(angle2) * m_lockOnDistance);
			p1.y = lockGroundY;
			p2.y = lockGroundY;

			gameContext.debugCollisionRenderer->QueueLine(p1, p2, lockOnRingColor);
		}
	}

	// - Draw 4 cardinal tick markers (North, South, East, West)
	const float tickLen = 1.5f;
	DirectX::XMVECTORF32 tickColor = { 1.0f, 0.6f, 0.1f, 0.95f };
	DirectX::SimpleMath::Vector3 directions[4] = {
		DirectX::SimpleMath::Vector3(1.0f, 0.0f, 0.0f),
		DirectX::SimpleMath::Vector3(-1.0f, 0.0f, 0.0f),
		DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f),
		DirectX::SimpleMath::Vector3(0.0f, 0.0f, -1.0f)
	};
	for (int d = 0; d < 4; ++d)
	{
		DirectX::SimpleMath::Vector3 tickStart = center + directions[d] * (m_triggerDistance - tickLen);
		DirectX::SimpleMath::Vector3 tickEnd = center + directions[d] * (m_triggerDistance + tickLen);
		tickStart.y = groundY;
		tickEnd.y = groundY;
		gameContext.debugCollisionRenderer->QueueLine(tickStart, tickEnd, tickColor);
	}
}

void HEIN::EncounterTriggerComponent::OnInspectorGUI(GameContext& gameContext)
{
	if (ImGui::CollapsingHeader("Encounter Trigger Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Enable Encounter Trigger", &m_enabled);
		ImGui::Checkbox("Draw Debug Range", &m_drawDebugRange);
		ImGui::Checkbox("Freeze Actors on Trigger", &m_freezeOnTrigger);
		ImGui::Checkbox("Trigger Victory on Death", &m_triggerVictoryOnDeath);

		ImGui::Checkbox("Switch to Lock-On at Cutscene End", &m_enableLockOnOnEnd);
		ImGui::Checkbox("Auto Lock-On on Proximity", &m_enableProximityLockOn);
		ImGui::DragFloat("Lock-On Proximity Distance (m)", &m_lockOnDistance, 0.5f, 2.0f, 1000.0f, "%.1f m");
		ImGui::DragFloat("Trigger Distance (m)", &m_triggerDistance, 0.5f, 2.0f, 1000.0f, "%.1f m");
		ImGui::DragFloat("AI Post-Cutscene Wake Delay (s)", &m_aiWakeDelay, 0.1f, 0.0f, 5.0f, "%.1f s");
		ImGui::DragFloat("Max Cutscene Duration (s)", &m_maxDuration, 0.1f, 1.0f, 30.0f, "%.1f s");

		char trackBuffer[64];
		strcpy_s(trackBuffer, m_cinematicTrackName.c_str());
		if (ImGui::InputText("Cinematic Track Name", trackBuffer, sizeof(trackBuffer)))
		{
			m_cinematicTrackName = trackBuffer;
		}

		char animBuffer[64];
		strcpy_s(animBuffer, m_introAnimName.c_str());
		if (ImGui::InputText("Intro Anim Name", animBuffer, sizeof(animBuffer)))
		{
			m_introAnimName = animBuffer;
		}

		ImGui::Separator();
		ImGui::Text("Status: Triggered = %s | Playing = %s (%.1fs)",
			m_hasTriggered ? "YES" : "NO",
			m_isPlayingCutscene ? "YES" : "NO",
			m_encounterTimer);

		if (ImGui::Button("Trigger Cutscene Now"))
		{
			StartEncounter(gameContext);
		}

		ImGui::SameLine();
		if (ImGui::Button("Skip Cutscene"))
		{
			SkipCutscene(gameContext);
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset Trigger"))
		{
			ResetEncounter();
		}
	}
}

nlohmann::json HEIN::EncounterTriggerComponent::Serialize()
{
	nlohmann::json data = IComponent::Serialize();
	data["Enabled"] = m_enabled;
	data["HasTriggered"] = m_hasTriggered;
	data["DrawDebugRange"] = m_drawDebugRange;
	data["FreezeOnTrigger"] = m_freezeOnTrigger;
	data["TriggerVictoryOnDeath"] = m_triggerVictoryOnDeath;
	data["TriggerDistance"] = m_triggerDistance;
	data["MaxDuration"] = m_maxDuration;
	data["CinematicTrackName"] = m_cinematicTrackName;
	data["IntroAnimName"] = m_introAnimName;
	data["EnableLockOnOnEnd"] = m_enableLockOnOnEnd;
	data["EnableProximityLockOn"] = m_enableProximityLockOn;
	data["LockOnDistance"] = m_lockOnDistance;
	data["AIWakeDelay"] = m_aiWakeDelay;
	return data;
}

void HEIN::EncounterTriggerComponent::Deserialize(const nlohmann::json& data)
{
	IComponent::Deserialize(data);

	if (data.contains("Enabled") && data["Enabled"].is_boolean())
		m_enabled = data["Enabled"].get<bool>();

	if (data.contains("HasTriggered") && data["HasTriggered"].is_boolean())
		m_hasTriggered = data["HasTriggered"].get<bool>();

	if (data.contains("DrawDebugRange") && data["DrawDebugRange"].is_boolean())
		m_drawDebugRange = data["DrawDebugRange"].get<bool>();

	if (data.contains("FreezeOnTrigger") && data["FreezeOnTrigger"].is_boolean())
		m_freezeOnTrigger = data["FreezeOnTrigger"].get<bool>();

	if (data.contains("TriggerVictoryOnDeath") && data["TriggerVictoryOnDeath"].is_boolean())
		m_triggerVictoryOnDeath = data["TriggerVictoryOnDeath"].get<bool>();

	if (data.contains("TriggerDistance") && data["TriggerDistance"].is_number())
		m_triggerDistance = data["TriggerDistance"].get<float>();

	if (data.contains("MaxDuration") && data["MaxDuration"].is_number())
		m_maxDuration = data["MaxDuration"].get<float>();

	if (data.contains("CinematicTrackName") && data["CinematicTrackName"].is_string())
		m_cinematicTrackName = data["CinematicTrackName"].get<std::string>();

	if (data.contains("IntroAnimName") && data["IntroAnimName"].is_string())
		m_introAnimName = data["IntroAnimName"].get<std::string>();

	if (data.contains("EnableLockOnOnEnd") && data["EnableLockOnOnEnd"].is_boolean())
		m_enableLockOnOnEnd = data["EnableLockOnOnEnd"].get<bool>();

	if (data.contains("EnableProximityLockOn") && data["EnableProximityLockOn"].is_boolean())
		m_enableProximityLockOn = data["EnableProximityLockOn"].get<bool>();

	if (data.contains("LockOnDistance") && data["LockOnDistance"].is_number())
		m_lockOnDistance = data["LockOnDistance"].get<float>();
	if (data.contains("AIWakeDelay") && data["AIWakeDelay"].is_number())
		m_aiWakeDelay = data["AIWakeDelay"].get<float>();
}
