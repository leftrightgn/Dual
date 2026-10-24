#pragma once
#include "../../External/Engine/Components/IComponent.h"
#include "../../External/Engine/Entities/Actor.h"
#include <string>

namespace HEIN
{
	class EncounterTriggerComponent : public IComponent
	{
	private:
		bool m_enabled = true;
		bool m_hasTriggered = false;
		bool m_drawDebugRange = true;
		bool m_freezeOnTrigger = true;
		bool m_triggerVictoryOnDeath = true;
		float m_triggerDistance = 70.0f;
		float m_maxDuration = 8.5f;
		float m_encounterTimer = 0.0f;
		bool m_isPlayingCutscene = false;

		std::string m_cinematicTrackName = "Boss_Intro";
		std::string m_introAnimName = "Idle";

		bool m_enableLockOnOnEnd = true;
		bool m_enableProximityLockOn = true;
		float m_lockOnDistance = 45.0f;

		bool m_isWakingUp = false;
		float m_wakeTimer = 0.0f;
		float m_aiWakeDelay = 1.2f;

	public:
		EncounterTriggerComponent(Actor* owner);

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
		std::string GetComponentName() const override { return "EncounterTriggerComponent"; }

		// - Trigger & Encounter Flow Control
		bool CheckAndTrigger(GameContext& gameContext, const DirectX::SimpleMath::Vector3& playerPos);
		void StartEncounter(GameContext& gameContext);
		void EndEncounter(GameContext& gameContext);
		void SkipCutscene(GameContext& gameContext);
		void ResetEncounter();
		void UpdateEncounter(float deltaTime, GameContext& gameContext);

		// - Freeze and Unfreeze Actors for Cutscene Stance
		void FreezeEncounterActors(GameContext& gameContext);
		void UnfreezeEncounterActors(GameContext& gameContext);

		// - Configuration Getters & Setters
		bool IsEnabled() const { return m_enabled; }
		void SetEnabled(bool enabled) { m_enabled = enabled; }

		bool HasTriggered() const { return m_hasTriggered; }
		void SetHasTriggered(bool triggered) { m_hasTriggered = triggered; }

		bool IsPlayingCutscene() const { return m_isPlayingCutscene; }

		float GetTriggerDistance() const { return m_triggerDistance; }
		void SetTriggerDistance(float dist) { m_triggerDistance = dist; }

		float GetMaxDuration() const { return m_maxDuration; }
		void SetMaxDuration(float dur) { m_maxDuration = dur; }

		float GetEncounterTimer() const { return m_encounterTimer; }

		bool IsDrawDebugRange() const { return m_drawDebugRange; }
		void SetDrawDebugRange(bool draw) { m_drawDebugRange = draw; }

		const std::string& GetCinematicTrackName() const { return m_cinematicTrackName; }
		void SetCinematicTrackName(const std::string& track) { m_cinematicTrackName = track; }

		const std::string& GetIntroAnimName() const { return m_introAnimName; }
		void SetIntroAnimName(const std::string& anim) { m_introAnimName = anim; }

		bool IsTriggerVictoryOnDeath() const { return m_triggerVictoryOnDeath; }
		void SetTriggerVictoryOnDeath(bool trigger) { m_triggerVictoryOnDeath = trigger; }

		bool IsFreezeOnTrigger() const { return m_freezeOnTrigger; }
		void SetFreezeOnTrigger(bool freeze) { m_freezeOnTrigger = freeze; }

		bool IsLockOnOnEndEnabled() const { return m_enableLockOnOnEnd; }
		void SetLockOnOnEndEnabled(bool enabled) { m_enableLockOnOnEnd = enabled; }

		bool IsProximityLockOnEnabled() const { return m_enableProximityLockOn; }
		void SetProximityLockOnEnabled(bool enabled) { m_enableProximityLockOn = enabled; }

		float GetLockOnDistance() const { return m_lockOnDistance; }
		void SetLockOnDistance(float dist) { m_lockOnDistance = dist; }

		bool IsWakingUp() const { return m_isWakingUp; }
		float GetAIWakeDelay() const { return m_aiWakeDelay; }
		void SetAIWakeDelay(float delay) { m_aiWakeDelay = delay; }
	};
}
