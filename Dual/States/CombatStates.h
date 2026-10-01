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

	std::unique_ptr<ICombatState> CreateCombatStateFromConfig(const StateConfig& config);

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
		bool IsBlockState() const override { return m_config.isBlock; }

		const StateConfig& GetConfig() const { return m_config; }
		float GetTimer() const { return m_timer; }
	};
	/**
	 * @class IdleState
	 * @brief Represents the neutral resting state of the actor.
	 * 
	 * Active when the actor is completely stationary (moveSpeed = 0). 
	 * Transitions instantly to WalkState on directional input, or immediately into 
	 * Action states (Attack, Dodge, Block) upon receiving the respective messages.
	 */
	class IdleState : public ICombatState
	{
	private:

		HEIN::StateConfig m_config;
	public:
		IdleState(const HEIN::StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;
		
	};

	/**
	 * @class WalkState
	 * @brief Represents the grounded movement state.
	 * 
	 * Active while movement input is held (moveSpeed = 30). Transitions back to 
	 * IdleState upon OnStop. Action states like Attack or Dodge can interrupt this state.
	 */
	class WalkState : public ICombatState
	{
	private:
		HEIN::StateConfig m_config;

	public:
		WalkState(const HEIN::StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;
	};

	/**
	 * @class OneHandAttackState
	 * @brief Handles the multi-stage melee combo system.
	 * 
	 * PURPOSE: Consolidates a 3-stage combo (Slash -> Cross -> Heavy) into a single state 
	 * using `m_comboStage` to track progression. 
	 * 
	 * COMBO STAGES:
	 * - Stage 1 (1.20s - 1.60s): Standard horizontal slash.
	 * - Stage 2 (2.20s - 2.40s): Fast returning slash.
	 * - Stage 3 (4.20s - 4.50s): High-damage overhead smash.
	 * 
	 * TRANSITIONS: The state remains active if the player inputs another attack within the 
	 * specific combo window timestamps (comboWindowStarts). If the window is missed, or the 
	 * final stage completes, it transitions back to IdleState. Can be canceled early into 
	 * DodgeState if an evade is buffered.
	 */
	class OneHandAttackState : public ICombatState
	{
	public:
		/// Fallback timing defaults used if no per-stage timings or model durations are available.
		static constexpr float STAGE_1_END_TIME     = 1.10f;
		static constexpr float STAGE_2_END_TIME     = 1.20f;
		static constexpr float STAGE_3_END_TIME     = 2.20f;
		static constexpr float STAGE_1_WINDOW_START = 0.40f;
		static constexpr float STAGE_2_WINDOW_START = 0.40f;
		static constexpr float STAGE_3_WINDOW_START = 0.60f;

	private:
		HEIN::StateConfig m_config;
		float m_timer = 0.0f;
		int m_comboStage = 0;
		bool m_comboQueued = false;

		/// @brief Total number of combo stages (3 or more dynamically supported).
		int GetTotalStages(Actor* owner = nullptr) const;

		/// @brief Animation name for a stage, or nullptr if not configured.
		const std::string* StageAnim(int stage) const;
		/// @brief Timestamp (seconds) at which a stage's attack motion concludes.
		float StageEndTime(int stage, Actor* owner = nullptr) const;
		/// @brief Timestamp (seconds) at which the next combo input starts being accepted.
		float StageWindowStart(int stage, Actor* owner = nullptr) const;
		/// @brief Crossfade duration (seconds) for blending INTO a stage.
		float StageBlend(int stage, float fallback = 0.1f) const;

		/// @brief Crossfades all skinned models into the given stage's animation.
		void PlayStage(Actor* owner, int stage, float blendDuration);

	public:
		OneHandAttackState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;

		bool IsAttackState() const override { return true; }

		int GetCurrentComboStage() const { return m_comboStage; }
		bool IsComboQueued() const { return m_comboQueued; }
	};

	/**
	 * @class DodgeState
	 * @brief Evasion state granting temporary invincibility.
	 * 
	 * Active upon OnDodge. 
	 * RESPONSIBILITY: Locks the actor's velocity to a fixed direction (`m_lockedDirection`) 
	 * and provides i-frames for the duration of the roll (typically 0.6s). 
	 * During this state, the actor ignores damage payloads in `DamageSystem::HandleTriggerHit`.
	 * Transitions to IdleState or WalkState upon completion.
	 */
	class DodgeState : public ICombatState
	{
	private:
		HEIN::StateConfig m_config;
		float m_timer = 0.0f;
		DirectX::SimpleMath::Vector3 m_lockedDirection;

	public:
		DodgeState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;
	};

	class StrafeState : public ICombatState
	{
	private:

		HEIN::StateConfig m_config;
	
		bool m_isRight;
	public:

		StrafeState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;
	};

	/**
	 * @class BlockState
	 * @brief Defensive posture for negating incoming damage.
	 * 
	 * Active while the block button (RMB) is held. 
	 * RESPONSIBILITY: Redirects incoming trigger damage to drain stamina instead of health. 
	 * If stamina is fully depleted, causes a guard break (transitioning back to IdleState).
	 */
	class BlockState : public ICombatState
	{
	private:

		HEIN::StateConfig m_config;

	public:

		BlockState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;

		bool IsBlockState() const override { return true; }
	};
}
