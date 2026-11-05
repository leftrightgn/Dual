#include "pch.h"
#include "GameFlowStateMachineComponent.h"
#include "BehaviourTreeComponent.h"
#include "CombatStateMachineComponent.h"
#include "CharacterMovementComponent.h"
#include "BlackBoard/BossBlackBoard.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "../../External/Engine/Camera/CameraController.h"
#include "../../External/Engine/Camera/CinematicMode.h"
#include "../../External/Engine/Components/HealthComponent.h"
#include "../../External/Engine/Components/RigidBodyComponent.h"
#include "../../External/Engine/Components/SkinnedModelComponent.h"
#include "../../External/Engine/Components/SoundComponent.h"
#include "../../External/Engine/Components/TransformComponent.h"
#include "../../External/Engine/Framework/GameContext.h"
#include "../../External/Engine/System/AudioSystem.h"
#if HEIN_WITH_EDITOR
#include "../../External/Engine/ImGui/imgui.h"
#include "../../External/Engine/ImGui/imgui_stdlib.h"
#endif

namespace HEIN
{
HEIN::GameFlowStateMachineComponent::GameFlowStateMachineComponent(Actor* owner)
	: IComponent(owner)
	, m_currentState(GameFlowState::Gameplay)
	, m_stateTimer(0.0f)
	, m_hasPlayedIntro(false)
	, m_enableIntroCinematic(true)
	, m_drawDebugTriggerRange(true)
	, m_bossIntroTriggerDistance(30.0f)
	, m_introMaxDuration(8.5f)
	, m_bossIntroAnim("Idle")
	, m_introTrackName("Boss_Intro")
	, m_isVictory(false)
	, m_isGameOver(false)
{
}

void HEIN::GameFlowStateMachineComponent::Start()
{
}

void HEIN::GameFlowStateMachineComponent::Update(float deltaTime)
{
	m_stateTimer += deltaTime;
}

void HEIN::GameFlowStateMachineComponent::Draw(
	GameContext& gameContext,
	const DirectX::SimpleMath::Matrix& /*world*/,
	const DirectX::SimpleMath::Matrix& /*view*/,
	const DirectX::SimpleMath::Matrix& /*proj*/
)
{
	// - Visually render boss intro trigger range volume when debug display is active
	if (!m_drawDebugTriggerRange) return;
	if (gameContext.debugCollisionRenderer == nullptr) return;

	// - Resolve center position for encounter trigger
	DirectX::SimpleMath::Vector3 center(0.0f, 0.0f, 0.0f);
	bool foundCenter = false;

	if (GetOwner())
	{
		if (auto* transform = GetOwner()->GetComponent<HEIN::TransformComponent>())
		{
			center = transform->GetPosition();
			foundCenter = true;
		}
	}

	if (!foundCenter && gameContext.actorManager)
	{
		if (auto* targetActor = gameContext.actorManager->GetFirstActorByType(ActorType::Enemy))
		{
			if (auto* transform = targetActor->GetComponent<HEIN::TransformComponent>())
			{
				center = transform->GetPosition();
				foundCenter = true;
			}
		}
	}

	if (!foundCenter) return;

	// - Queue 3D Cyan wireframe bounding sphere for trigger boundary volume
	DirectX::BoundingSphere sphere(center, m_bossIntroTriggerDistance);
	DirectX::XMVECTORF32 cyanColor = { 0.0f, 0.85f, 1.0f, 0.85f };
	gameContext.debugCollisionRenderer->QueueSphere(sphere, cyanColor);

	// - Queue 2D Yellow ground-level circular perimeter ring (64 segments)
	const int segments = 64;
	DirectX::XMVECTORF32 yellowColor = { 1.0f, 0.9f, 0.15f, 0.95f };
	float groundY = center.y + 0.15f;

	for (int i = 0; i < segments; ++i)
	{
		float angle1 = (DirectX::XM_2PI / segments) * static_cast<float>(i);
		float angle2 = (DirectX::XM_2PI / segments) * static_cast<float>(i + 1);

		DirectX::SimpleMath::Vector3 p1 = center + DirectX::SimpleMath::Vector3(cosf(angle1) * m_bossIntroTriggerDistance, 0.0f, sinf(angle1) * m_bossIntroTriggerDistance);
		DirectX::SimpleMath::Vector3 p2 = center + DirectX::SimpleMath::Vector3(cosf(angle2) * m_bossIntroTriggerDistance, 0.0f, sinf(angle2) * m_bossIntroTriggerDistance);
		p1.y = groundY;
		p2.y = groundY;

		gameContext.debugCollisionRenderer->QueueLine(p1, p2, yellowColor);
	}

	// - Queue 4 cardinal tick lines (North, South, East, West) extending across the perimeter
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
		DirectX::SimpleMath::Vector3 tickStart = center + directions[d] * (m_bossIntroTriggerDistance - tickLen);
		DirectX::SimpleMath::Vector3 tickEnd = center + directions[d] * (m_bossIntroTriggerDistance + tickLen);
		tickStart.y = groundY;
		tickEnd.y = groundY;
		gameContext.debugCollisionRenderer->QueueLine(tickStart, tickEnd, tickColor);
	}
}

void HEIN::GameFlowStateMachineComponent::FreezeBossAndPlayer(GameContext& gameContext)

{

	if (!gameContext.actorManager) return;



	// Freeze Boss AI and stop motion

	if (auto* enemy = gameContext.actorManager->GetFirstActorByType(ActorType::Enemy))

	{

		if (auto* bt = enemy->GetComponent<BehaviourTreeComponent>())

		{

			bt->SetPaused(true);

		}

		if (auto* bb = enemy->GetComponent<BossBlackBoard>())

		{

			bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;

			bb->isLockedOn = false;

			bb->isAttacking = false;

			bb->activeNodeName = "Cinematic (Paused)";

		}

		if (auto* rb = enemy->GetComponent<RigidBodyComponent>())

		{

			rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);

		}

		if (auto* cm = enemy->GetComponent<CharacterMovementComponent>())

		{

			cm->ClearImpulse();

		}

		if (auto* combat = enemy->GetComponent<CombatStateMachineComponent>())

		{

			if (combat->GetCurrentStateName() != "Idle")

			{

				combat->ChangeState("Idle", 0.1f);

			}

		}

		if (auto* model = enemy->GetComponent<SkinnedModelComponent>())

		{

			if (model->HasAnimation(m_bossIntroAnim))

			{

				model->ChangeAnimation(m_bossIntroAnim);

			}

		}

	}



	// Freeze Player movement and clear combat intent

	if (auto* player = gameContext.actorManager->GetFirstActorByType(ActorType::Player))

	{

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

	}

}



void HEIN::GameFlowStateMachineComponent::UnfreezeBossAndPlayer(GameContext& gameContext)

{

	if (!gameContext.actorManager) return;



	// Unpause Boss Behaviour Tree to resume combat

	if (auto* enemy = gameContext.actorManager->GetFirstActorByType(ActorType::Enemy))

	{

		if (auto* bt = enemy->GetComponent<BehaviourTreeComponent>())

		{

			bt->SetPaused(false);

		}

		if (auto* model = enemy->GetComponent<SkinnedModelComponent>())

		{

			if (model->HasAnimation("Idle"))

			{

				model->ChangeAnimation("Idle");

			}

		}

	}

}



void HEIN::GameFlowStateMachineComponent::ChangeState(GameFlowState newState, GameContext& gameContext)

{

	m_currentState = newState;

	m_stateTimer = 0.0f;



	switch (m_currentState)

	{

	case GameFlowState::Intro:

		m_hasPlayedIntro = true;

		FreezeBossAndPlayer(gameContext);

		if (GetOwner())
		{
			if (auto* soundComp = GetOwner()->GetComponent<SoundComponent>())
			{
				soundComp->PlayCueOrFallbackPath(m_introBgmCue, AudioBus::BGM, 1.0f);
			}
		}

		if (gameContext.mainCamera)

		{

			gameContext.mainCamera->RequestSwitch(CameraType::Cinematic);

			gameContext.mainCamera->ApplyRequest();

			if (auto* cinematic = dynamic_cast<CinematicMode*>(gameContext.mainCamera->GetCurrentCameraMode()))

			{

				cinematic->PlayTrack(m_introTrackName);

			}

		}

		break;



	case GameFlowState::Gameplay:

		UnfreezeBossAndPlayer(gameContext);

		if (GetOwner())
		{
			if (auto* soundComp = GetOwner()->GetComponent<SoundComponent>())
			{
				soundComp->PlayCueOrFallbackPath(m_gameplayBgmCue, AudioBus::BGM, 1.0f);
			}
		}

		if (gameContext.mainCamera)

		{

			gameContext.mainCamera->RequestSwitch(CameraType::Spring);

			gameContext.mainCamera->UpdateMouseMode();

		}

		break;



	case GameFlowState::Victory:

		TriggerVictory(gameContext);

		if (GetOwner())
		{
			if (auto* soundComp = GetOwner()->GetComponent<SoundComponent>())
			{
				soundComp->PlayCueOrFallbackPath(m_victoryBgmCue, AudioBus::BGM, 1.0f);
			}
		}

		break;



	case GameFlowState::GameOver:

		TriggerGameOver(gameContext);

		if (GetOwner())
		{
			if (auto* soundComp = GetOwner()->GetComponent<SoundComponent>())
			{
				soundComp->PlayCueOrFallbackPath(m_gameOverBgmCue, AudioBus::BGM, 1.0f);
			}
		}

		break;

	}

}



void HEIN::GameFlowStateMachineComponent::SkipIntro(GameContext& gameContext)

{

	if (m_currentState == GameFlowState::Intro)

	{

		if (gameContext.mainCamera)

		{

			if (auto* cinematic = dynamic_cast<CinematicMode*>(gameContext.mainCamera->GetCurrentCameraMode()))

			{

				cinematic->Skip();

			}

		}

		ChangeState(GameFlowState::Gameplay, gameContext);

	}

}



void HEIN::GameFlowStateMachineComponent::TriggerVictory(GameContext& /*gameContext*/)

{

	m_isVictory = true;

	DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);

	while (ShowCursor(TRUE) < 0);

}



void HEIN::GameFlowStateMachineComponent::TriggerGameOver(GameContext& /*gameContext*/)

{

	m_isGameOver = true;

	DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);

	while (ShowCursor(TRUE) < 0);

}



void HEIN::GameFlowStateMachineComponent::ResetMatch(GameContext& gameContext)

{

	m_isVictory = false;

	m_isGameOver = false;

	m_hasPlayedIntro = false;

	if (AudioSystem* audio = AudioSystem::GetInstance())
	{
		audio->ResetAllDucking();
	}

	ChangeState(GameFlowState::Gameplay, gameContext);

}



#if HEIN_WITH_EDITOR
void HEIN::GameFlowStateMachineComponent::OnInspectorGUI(GameContext& gameContext)

{

	if (ImGui::CollapsingHeader("Game Flow State Machine", ImGuiTreeNodeFlags_DefaultOpen))

	{

		const char* stateNames[] = { "Intro", "Gameplay", "Victory", "GameOver" };

		int currentIdx = static_cast<int>(m_currentState);



		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "[CURRENT STATE: %s]", stateNames[currentIdx]);

		ImGui::Text("State Timer: %.2f s", m_stateTimer);

		ImGui::Separator();



		ImGui::Text("State Transitions:");

		if (ImGui::Button("Trigger Intro"))

		{

			ChangeState(GameFlowState::Intro, gameContext);

		}

		ImGui::SameLine();

		if (ImGui::Button("Trigger Gameplay"))

		{

			ChangeState(GameFlowState::Gameplay, gameContext);

		}

		ImGui::SameLine();

		if (ImGui::Button("Trigger Victory"))

		{

			ChangeState(GameFlowState::Victory, gameContext);

		}

		ImGui::SameLine();

		if (ImGui::Button("Trigger GameOver"))

		{

			ChangeState(GameFlowState::GameOver, gameContext);

		}



		ImGui::Separator();

		ImGui::Text("Intro & Cinematic Configuration:");

		ImGui::Checkbox("Enable Intro Cinematic", &m_enableIntroCinematic);

		ImGui::Checkbox("Draw Trigger Range (Debug Mode)", &m_drawDebugTriggerRange);

		ImGui::DragFloat("Intro Trigger Distance (m)", &m_bossIntroTriggerDistance, 0.5f, 5.0f, 1000.0f, "%.1f m");

		ImGui::DragFloat("Max Intro Duration (s)", &m_introMaxDuration, 0.1f, 1.0f, 30.0f, "%.1f s");



		char animBuffer[64];

		strcpy_s(animBuffer, m_bossIntroAnim.c_str());

		if (ImGui::InputText("Boss Intro Anim Name", animBuffer, sizeof(animBuffer)))

		{

			m_bossIntroAnim = animBuffer;

		}



		if (ImGui::Button("Anim: Idle")) m_bossIntroAnim = "Idle";

		ImGui::SameLine();

		if (ImGui::Button("Anim: Walk")) m_bossIntroAnim = "Walk";

		ImGui::SameLine();

		if (ImGui::Button("Anim: HeavyAttack")) m_bossIntroAnim = "HeavyAttack";



		char trackBuffer[64];

		strcpy_s(trackBuffer, m_introTrackName.c_str());

		if (ImGui::InputText("Intro Cutscene Track Name", trackBuffer, sizeof(trackBuffer)))

		{

			m_introTrackName = trackBuffer;

		}



		ImGui::Separator();

		if (ImGui::Button("Skip Current Intro"))

		{

			SkipIntro(gameContext);

		}

		ImGui::SameLine();

		if (ImGui::Button("Reset Match State"))

		{

			ResetMatch(gameContext);

		}

		ImGui::Separator();
		if (ImGui::TreeNodeEx("Game Flow BGM Cues (SoundComponent)", ImGuiTreeNodeFlags_DefaultOpen))
		{
			SoundComponent* soundComp = GetOwner() ? GetOwner()->GetComponent<SoundComponent>() : nullptr;
			if (!soundComp)
			{
				ImGui::TextDisabled("No SoundComponent attached to GameManager Actor yet.");
				if (GetOwner() && ImGui::Button("+ Attach SoundComponent with BGM Preset"))
				{
					soundComp = GetOwner()->AddComponent<SoundComponent>();
					soundComp->ApplyBGMPreset();
					soundComp->Start();
				}
			}
			else
			{
				ImGui::InputText("Intro BGM Cue", &m_introBgmCue);
				ImGui::InputText("Gameplay BGM Cue", &m_gameplayBgmCue);
				ImGui::InputText("Victory BGM Cue", &m_victoryBgmCue);
				ImGui::InputText("GameOver BGM Cue", &m_gameOverBgmCue);
			}
			ImGui::TreePop();
		}

	}

}
#else
void HEIN::GameFlowStateMachineComponent::OnInspectorGUI(GameContext& /*gameContext*/) {}
#endif



nlohmann::json HEIN::GameFlowStateMachineComponent::Serialize()

{

	nlohmann::json data = IComponent::Serialize();

	data["CurrentState"] = static_cast<int>(m_currentState);

	data["EnableIntroCinematic"] = m_enableIntroCinematic;

	data["DrawDebugTriggerRange"] = m_drawDebugTriggerRange;

	data["BossIntroTriggerDistance"] = m_bossIntroTriggerDistance;

	data["IntroMaxDuration"] = m_introMaxDuration;

	data["BossIntroAnim"] = m_bossIntroAnim;

	data["IntroTrackName"] = m_introTrackName;

	data["HasPlayedIntro"] = m_hasPlayedIntro;

	data["IntroBgmCue"] = m_introBgmCue;
	data["GameplayBgmCue"] = m_gameplayBgmCue;
	data["VictoryBgmCue"] = m_victoryBgmCue;
	data["GameOverBgmCue"] = m_gameOverBgmCue;

	return data;

}



void HEIN::GameFlowStateMachineComponent::Deserialize(const nlohmann::json& data)

{

	IComponent::Deserialize(data);

	if (data.contains("CurrentState") && data["CurrentState"].is_number())

		m_currentState = static_cast<GameFlowState>(data["CurrentState"].get<int>());

	if (data.contains("EnableIntroCinematic") && data["EnableIntroCinematic"].is_boolean())

		m_enableIntroCinematic = data["EnableIntroCinematic"].get<bool>();

	if (data.contains("DrawDebugTriggerRange") && data["DrawDebugTriggerRange"].is_boolean())

		m_drawDebugTriggerRange = data["DrawDebugTriggerRange"].get<bool>();

	if (data.contains("BossIntroTriggerDistance") && data["BossIntroTriggerDistance"].is_number())

		m_bossIntroTriggerDistance = data["BossIntroTriggerDistance"].get<float>();

	if (data.contains("IntroMaxDuration") && data["IntroMaxDuration"].is_number())

		m_introMaxDuration = data["IntroMaxDuration"].get<float>();

	if (data.contains("BossIntroAnim") && data["BossIntroAnim"].is_string())

		m_bossIntroAnim = data["BossIntroAnim"].get<std::string>();

	if (data.contains("HasPlayedIntro") && data["HasPlayedIntro"].is_boolean())

		m_hasPlayedIntro = data["HasPlayedIntro"].get<bool>();

	if (data.contains("IntroBgmCue") && data["IntroBgmCue"].is_string())
		m_introBgmCue = data["IntroBgmCue"].get<std::string>();
	if (data.contains("GameplayBgmCue") && data["GameplayBgmCue"].is_string())
		m_gameplayBgmCue = data["GameplayBgmCue"].get<std::string>();
	if (data.contains("VictoryBgmCue") && data["VictoryBgmCue"].is_string())
		m_victoryBgmCue = data["VictoryBgmCue"].get<std::string>();
	if (data.contains("GameOverBgmCue") && data["GameOverBgmCue"].is_string())
		m_gameOverBgmCue = data["GameOverBgmCue"].get<std::string>();

}
} // namespace HEIN
