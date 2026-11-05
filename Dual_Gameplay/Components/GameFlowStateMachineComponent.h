#pragma once
#include "../../External/Engine/Components/IComponent.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"
#include <string>

namespace HEIN
{
	enum class GameFlowState
	{
		Intro,
		Gameplay,
		Victory,
		GameOver
	};

	class GameFlowStateMachineComponent : public IComponent
	{
	private:
		GameFlowState m_currentState = GameFlowState::Intro;
		float m_stateTimer = 0.0f;
		bool m_hasPlayedIntro = false;
		bool m_enableIntroCinematic = true;
		bool m_drawDebugTriggerRange = true;
		float m_bossIntroTriggerDistance = 30.0f;
		float m_introMaxDuration = 8.5f;
		std::string m_bossIntroAnim = "Idle";
		std::string m_introTrackName = "Boss_Intro";

		// BGM SoundComponent Cue Bindings per GameFlowState
		std::string m_introBgmCue = "BGM_Intro";
		std::string m_gameplayBgmCue = "BGM_Arena";
		std::string m_victoryBgmCue = "BGM_Victory";
		std::string m_gameOverBgmCue = "BGM_GameOver";

		// Result UI feedback state
		bool m_isVictory = false;
		bool m_isGameOver = false;

	public:
		GameFlowStateMachineComponent(Actor* owner);

		void Start() override;
		void Update(float deltaTime) override;
		void Draw(
			GameContext& gameContext,
			const DirectX::SimpleMath::Matrix& world,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj
		) override;
		void OnInspectorGUI(GameContext& gameContext) override;

		nlohmann::json Serialize() override;
		void Deserialize(const nlohmann::json& data) override;
		std::string GetComponentName() const override { return "GameFlowStateMachineComponent"; }

		// State Machine API
		GameFlowState GetCurrentState() const { return m_currentState; }
		void ChangeState(GameFlowState newState, GameContext& gameContext);

		void TriggerGameOver(GameContext& gameContext);
		void TriggerVictory(GameContext& gameContext);
		void ResetMatch(GameContext& gameContext);
		void SkipIntro(GameContext& gameContext);

		// Freeze / Unfreeze helpers for Boss AI & Player movement
		void FreezeBossAndPlayer(GameContext& gameContext);
		void UnfreezeBossAndPlayer(GameContext& gameContext);

		// Configuration Getters & Setters
		bool IsIntroCinematicEnabled() const { return m_enableIntroCinematic; }
		void SetIntroCinematicEnabled(bool enabled) { m_enableIntroCinematic = enabled; }
		bool IsDrawDebugTriggerRange() const { return m_drawDebugTriggerRange; }
		void SetDrawDebugTriggerRange(bool draw) { m_drawDebugTriggerRange = draw; }
		float GetBossIntroTriggerDistance() const { return m_bossIntroTriggerDistance; }
		void SetBossIntroTriggerDistance(float dist) { m_bossIntroTriggerDistance = dist; }
		float GetIntroMaxDuration() const { return m_introMaxDuration; }
		void SetIntroMaxDuration(float dur) { m_introMaxDuration = dur; }
		const std::string& GetBossIntroAnim() const { return m_bossIntroAnim; }
		void SetBossIntroAnim(const std::string& anim) { m_bossIntroAnim = anim; }
		const std::string& GetIntroTrackName() const { return m_introTrackName; }
		void SetIntroTrackName(const std::string& name) { m_introTrackName = name; }

		bool HasPlayedIntro() const { return m_hasPlayedIntro; }
		void SetHasPlayedIntro(bool played) { m_hasPlayedIntro = played; }
	};
}
