#pragma once
#include "States/ICombatState.h"
#include "../../../External/Engine/Entities/Actor.h"
#include <string>
#include <unordered_map>
#include <vector>
namespace HEIN
{
	struct TransitionData
	{
		std::string targetState;
		float blendDuration = 0.2f;
		std::string triggerEvent = ""; // "OnAttack", "OnForwardAttack", "OnDodge", "OnBlock", "OnStopBlock", "OnMove", "OnStop", "OnStrafe", "OnFinished"
		bool hasExitTime = false;      // Automatically transitions when state animation concludes or stateDuration elapsed
		float windowStart = 0.0f;      // Window timestamp (seconds) when input is accepted
		float windowEnd = 999.0f;      // Window timestamp (seconds) until input is accepted
		bool canInterrupt = false;     // Can interrupt from earlier stages/states
	};

	/**
	 * @struct StateConfig
	 * @brief Unified configuration data for defining ANY combat state purely via data.
	 * 
	 * Contains animation names, movement speed, timing windows, stances, and transition links.
	 * Enables joining state-to-state with custom triggers, combo windows, and exit times.
	 */
	struct StateConfig
	{
		std::string stateName = "Idle";
		std::string stateType = "DataDriven";

		/// @brief The primary animation to play during this state.
		std::string animationName = "Idle";
		
		/// @brief An optional secondary animation (e.g. for lower-body movement).
		std::string secondaryAnimationName = "";

		/// @brief The actor's movement speed while in this state.
		float moveSpeed = 0.0f;
		
		/// @brief The total duration of this state (0.0f = auto-detect from animation clip length).
		float stateDuration = 0.0f;

		/// @brief How quickly the actor can rotate during this state.
		float turnSpeed = 10.0f;

		/// @brief True for looping states (Idle, Walk, Strafe), false for one-shots (Attacks, Dodge).
		bool isLooping = false;

		/// @brief If true, activates weapon damage dealer hitboxes during this state.
		bool isAttack = false;

		/// @brief If true, takes defensive stance and absorbs damage using stamina.
		bool isBlock = false;

		/// @brief If true, locks actor's movement direction to facing/input intent on enter (for Dodge).
		bool lockMovementDirection = false;

		/// @brief Timestamp (seconds) where invincibility starts (i-frames).
		float invincibilityStart = 0.0f;

		/// @brief Timestamp (seconds) where invincibility ends.
		float invincibilityEnd = 0.0f;

		/// @brief Maps an input or event key to destination state transition data.
		std::unordered_map<std::string, TransitionData> transitions;

		// Legacy combo fields (kept for backward compatibility)
		std::vector<std::string> comboAnimationNames;
		std::vector<float> comboEndTimes;
		std::vector<float> comboWindowStarts;
		std::vector<float> comboBlendDurations;
		float comboExitBlendDuration = 0.3f;

		size_t GetComboStageCount() const
		{
			if (!comboAnimationNames.empty()) return comboAnimationNames.size();
			if (!comboEndTimes.empty()) return comboEndTimes.size();
			if (!animationName.empty()) return 1;
			return 0;
		}
	};

	class CombatStateMachineComponent;

	/**
	 * @class UniversalCombatState
	 * @brief Unified, data-driven combat state capable of handling ANY state in the game.
	 * 
	 * Eliminates hardcoded C++ state classes by reading state properties, timing windows,
	 * animation crossfades, and state-to-state transitions entirely from StateConfig data.
	 */
	class UniversalCombatState : public ICombatState
	{
	private:
		StateConfig m_config;
		float m_timer = 0.0f;
		float m_actualDuration = 1.0f;
		DirectX::SimpleMath::Vector3 m_lockedDirection = DirectX::SimpleMath::Vector3::Zero;

		bool m_hasQueuedTransition = false;
		TransitionData m_queuedTransition;
		float m_stopDebounceTimer = 0.0f;

	public:
		UniversalCombatState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration) override;
		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;
		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;
		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;

		bool IsAttackState() const override { return m_config.isAttack; }
		bool IsBlockState() const override { return m_config.isBlock || m_config.stateName == "Block" || m_config.stateType == "Block"; }

		const StateConfig& GetConfig() const { return m_config; }
		float GetTimer() const { return m_timer; }
	};

	
}
