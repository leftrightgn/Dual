#pragma once
#include "States/ICombatState.h"
#include "../../../External/Engine/Entities/Actor.h"
#include <string>
#include <unordered_map>
#include <vector>
namespace HEIN
{
	/**
	 * @struct StateConfig
	 * @brief Configuration data for initializing a combat state.
	 * 
	 * Contains shared parameters such as animation names, transition mappings,
	 * timing windows for combos, and movement modifiers for the state.
	 */
	/**
	 * @struct StateConfig
	 * @brief Configuration data for an individual combat state.
	 * 
	 * Contains animation names, transition logic, combo timings, and movement modifiers
	 * needed to execute a specific combat state (like an attack, block, or dodge).
	 */
	struct StateConfig
	{
		/// @brief The primary animation to play during this state.
		std::string animationName;
		
		/// @brief An optional secondary animation (e.g. for lower-body movement).
		std::string secondaryAnimationName;

		/// @brief Maps an input or event (key) to the name of the next state (value).
		std::unordered_map<std::string, std::string> transitions;

		/// @brief Timestamps indicating when this state can be canceled into a combo.
		std::vector<float> comboEndTimes;
		
		/// @brief Timestamps indicating the window during which combo inputs are accepted.
		std::vector<float> comboWindowStarts;

		/// @brief The actor's movement speed while in this state.
		float moveSpeed = 0.0f;
		
		/// @brief The total duration of this state before it ends automatically.
		float stateDuration = 1.0f;
	};

	class CombatStateMachineComponent;
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

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

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

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

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
		// PURPOSE: Defines the specific timestamps when combo stages start and end.
		// INTENT: Ensures that attacks flow logically with the underlying animations,
		// allowing players to perfectly time their inputs within these windows.
		// DESIGN DECISIONS: Extracted from config arrays to prevent magic numbers.
		static constexpr float STAGE_1_WINDOW_START = 1.20f;
		static constexpr float STAGE_1_END_TIME = 1.60f;

		static constexpr float STAGE_2_WINDOW_START = 2.20f;
		static constexpr float STAGE_2_END_TIME = 2.40f;

		static constexpr float STAGE_3_WINDOW_START = 4.20f;
		static constexpr float STAGE_3_END_TIME = 4.50f;

	private:
		HEIN::StateConfig m_config;
		float m_timer = 0.0f;
		int m_comboStage = 0;

	public:
		OneHandAttackState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;

		bool IsAttackState() const override { return true; }
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

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

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

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

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

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;

		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;

		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;

		bool IsBlockState() const override { return true; }
	};
}
