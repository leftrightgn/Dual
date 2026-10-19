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
		bool requireHit = false;       // Only valid if an attack hit has been confirmed
	};

	/**
	 * @struct StateConfig
	 * @brief Unified configuration data for defining ANY combat state purely via data.
	 * 
	 * Contains animation names, movement speed, timing windows, stances, and transition links.
	 * Enables joining state-to-state with custom triggers, combo windows, and exit times.
	 */
	enum class CombatStateType
	{
		Normal = 0,
		Attack = 1,
		ComboFinisher = 2,
		Block = 3,
		Dodge = 4,
		Stun = 5,
		Jump = 6,
		GuardCounter = 7
	};

	enum class StateEffectType
	{
		Shockwave = 0,
		CameraShake = 1,
		SoundCue = 2,
		VFX = 3
	};

	struct StateEffectConfig
	{
		StateEffectType type = StateEffectType::Shockwave;
		float triggerTime = 0.0f;
		mutable bool hasTriggered = false;

		// Shockwave properties
		float radius = 24.0f;
		float duration = 1.1f;
		float damage = 45.0f;
		float knockbackForce = 35.0f;
		float forwardOffset = 18.0f;
		bool isGroundOnly = true;

		// Camera Shake properties
		float shakeIntensity = 0.5f;
		float shakeDuration = 0.3f;

		// Asset identifier
		std::string assetKey = "";
	};

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

		/// @brief If true, marks state as a high-impact Guard Counter attack.
		bool isGuardCounter = false;

		/// @brief If true, locks actor's movement direction to facing/input intent on enter (for Dodge).
		bool lockMovementDirection = false;

		/// @brief Timestamp (seconds) where invincibility starts (i-frames).
		float invincibilityStart = 0.0f;

		/// @brief Timestamp (seconds) where invincibility ends.
		float invincibilityEnd = 0.0f;

		// --- Souls-like Combat Feel: Anticipation Wind-Up Pause & Hitstop ---
		/// @brief Timestamp (seconds) into the attack clip when the wind-up reaches its apex and pauses.
		float windupPauseStart = 0.16f;

		/// @brief Real-time duration (seconds) to pause/hold at the wind-up apex before active frames (0.0f = disabled).
		float windupPauseDuration = 0.0f;

		/// @brief Animation time-scale during the wind-up pause (0.0f = complete interpolation freeze at apex).
		float windupTimeScale = 0.0f;

		/// @brief Animation speed multiplier during the active strike release after wind-up (e.g. 1.18f for snappy release).
		float strikeSpeedMultiplier = 1.15f;

		/// @brief Hitstop freeze duration in fixed 60fps frames applied to both attacker & defender on hit.
		int hitstopFrames = 5;

		/// @brief Active weapon hitbox window start/end in animation time (0.0f = auto-activate after wind-up pause).
		float hitboxStart = 0.0f;
		float hitboxEnd = 0.0f;

		// --- Data-Driven Ground Shockwave Parameters ---
		bool emitShockwave = false;
		float shockwaveRadius = 24.0f;
		float shockwaveDuration = 1.1f;
		float shockwaveDamage = 45.0f;
		float shockwaveKnockback = 35.0f;
		float shockwaveForwardOffset = 18.0f;
		bool shockwaveIsGroundOnly = true;

		// --- Data-Driven Combat System Refactoring ---
		CombatStateType stateTypeEnum = CombatStateType::Normal;
		float staminaCost = 0.0f;
		float staminaDamage = 20.0f;
		int comboPermutationStep = -1;
		std::vector<StateEffectConfig> effects;

		bool IsAttackState() const
		{
			return stateTypeEnum == CombatStateType::Attack ||
			       stateTypeEnum == CombatStateType::ComboFinisher ||
			       stateTypeEnum == CombatStateType::GuardCounter ||
			       isAttack;
		}

		bool IsBlockState() const
		{
			return stateTypeEnum == CombatStateType::Block || isBlock;
		}

		bool IsDodgeState() const
		{
			return stateTypeEnum == CombatStateType::Dodge || lockMovementDirection || invincibilityEnd > 0.0f;
		}

		bool IsStunState() const
		{
			return stateTypeEnum == CombatStateType::Stun;
		}

		bool IsJumpState() const
		{
			return stateTypeEnum == CombatStateType::Jump;
		}

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
		const StateConfig* m_config = nullptr;
		float m_timer = 0.0f;
		float m_actualDuration = 1.0f;
		DirectX::SimpleMath::Vector3 m_lockedDirection = DirectX::SimpleMath::Vector3::Zero;

		// Anticipation Wind-Up Pause & Active Hitbox state
		float m_windupHoldTimer = 0.0f;
		bool m_hasTriggeredWindupPause = false;
		bool m_isInWindupPause = false;
		bool m_isWeaponHitboxActive = false;
		bool m_hasSpawnedShockwave = false;

		bool m_hasQueuedTransition = false;
		TransitionData m_queuedTransition;
		float m_stopDebounceTimer = 0.0f;

	public:
		UniversalCombatState(const StateConfig* config);
		UniversalCombatState(const StateConfig& config);

		void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration) override;
		void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) override;
		void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) override;
		bool HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID) override;

		bool IsAttackState() const override { return m_config ? m_config->isAttack : false; }
		bool IsWeaponHitboxActive() const override { return m_config ? (m_config->isAttack && m_isWeaponHitboxActive) : false; }
		int GetHitstopFrames() const override { return m_config ? m_config->hitstopFrames : 5; }
		bool IsBlockState() const override { return m_config ? m_config->IsBlockState() : false; }
		bool IsGuardCounterState() const override { return m_config ? m_config->isGuardCounter : false; }

		const StateConfig& GetConfig() const { return *m_config; }
		float GetTimer() const { return m_timer; }
		bool IsInWindupPause() const { return m_isInWindupPause; }
	};

	
}
