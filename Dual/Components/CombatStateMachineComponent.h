#pragma once
#include "../../External/Engine/Common/Event.h"
#include "../../External/Engine/Components/IComponent.h"
#include "../../External/Engine/Message/IObserver.h"
#include "../../External/Engine/Message/Message.h"
#include <States/CombatStates.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include "../../External/Engine/Common/json.hpp"

namespace HEIN
{
	class Actor;
	class ICombatState;

	/**
	 * @class CombatStateMachineComponent
	 * @brief Component responsible for managing and executing combat states for an Actor.
	 * 
	 * DESIGN DECISION: the architecture decouples the FSM into its own component rather than embedding it 
	 * inside PlayerInputComponent or BehaviourTreeComponent. This allows both player-controlled 
	 * avatars and AI-driven enemies to share the exact same animation chaining, hit-frame logic, 
	 * and state transitions.
	 * 
	 * The state machine relies on a 200ms input buffering window (`m_messageBuffer`). 
	 * PURPOSE: Human reaction time and animation locks make frame-perfect inputs frustrating. 
	 * Buffering inputs ensures that if a player presses "Attack" or "Dodge" right before the 
	 * current animation ends, the action will chain seamlessly rather than being ignored.
	 * 
	 * COMBO CHAINING: Attack sequences rely on tight time windows (e.g. Stage 1 to Stage 2). 
	 * The buffer queue is polled continuously during these valid windows. If a mapped transition 
	 * exists (like ACTION_ATTACK -> OneHandAttackState) and an input was buffered, the state 
	 * automatically advances without dropping the combo flow.
	 */
	class CombatStateMachineComponent : public IComponent, public IObserver
	{
	private:
		/// @brief The currently active combat state.
		ICombatState* m_currentState = nullptr;
		
		/// @brief The next state to transition to, queued up for processing.
		ICombatState* m_pendingState = nullptr;

		float m_pendingBlendDuration = 0.2f;

		std::string m_currentStateName = "Idle";
		std::string m_defaultStateName = "Idle";
		/// @brief A map of all available states, keyed by their string name.
		std::unordered_map<std::string, std::unique_ptr<ICombatState>> m_states;

		std::unordered_map<std::string, StateConfig> m_stateConfigs;

		/// @brief Buffer storing incoming messages/inputs for combat (e.g. for combos).
		std::vector<Message::MessageID> m_messageBuffer;
		
		/// @brief The maximum duration (in seconds) to hold messages in the buffer.
		/// PURPOSE: Sets a 200ms window to buffer player inputs (e.g. 0.2f). 
		/// INTENT: Human reaction times make frame-perfect inputs frustrating. 
		/// Buffering inputs allows actions to chain seamlessly without dropping them, 
		/// maintaining the fluid feel of the combat.
		static constexpr float MAX_BUFFER_TIME = 0.2f;

		/// @brief Timer tracking how long the current messages have been buffered.
		float m_bufferTime = 0.0f;

		/**
		 * @brief Path to the standalone combat configuration asset JSON (e.g. "Resources/Data/Combat/Knight_Combat.json").
		 * 
		 * ARCHITECTURAL DECISION:
		 * Decouples combat state machine definitions (combos, windows, hit timings) into external, dedicated JSON assets.
		 * 1. Isolates combat tuning from world placement: Scene files (GameScene.json) stay lightweight and clean.
		 * 2. Prevents runtime state corruption: Saving a scene mid-fight never overwrites or pollutes the combat asset.
		 * 3. Reusability: Multiple enemies/archetypes can reference the exact same combat asset file.
		 */
		std::string m_configPath = "";

	public:
		CombatStateMachineComponent(Actor* owner);
	
		void Start() override;

		void Update(float deltaTime) override;


		std::string GetComponentName() const override { return "CombatStateMachineComponent"; }
		nlohmann::json Serialize() override;
		void Deserialize(const nlohmann::json& data) override;
		void OnInspectorGUI(GameContext& gameContext) override;
		/**
		 * @brief Requests a transition to a new combat state.
		 * @param stateName The string identifier for the target state.
		 */
		void ChangeState(const std::string& stateName, float blendDuration = 0.2f);

		/**
		 * @brief Applies the queued m_pendingState, calling OnExit for the old 
		 *        state and OnEnter for the new state.
		 */
		void ApplyPendingState();
		
		/**
		 * @brief Registers a new state with the state machine.
		 * @param stateName The name used to look up this state.
		 * @param state Unique pointer to the instantiated state object.
		 */
		void AddState(const std::string& stateName, std::unique_ptr<ICombatState> state, const StateConfig& config);
		void AddState(const StateConfig& config);

		/**
		 * @brief Handles incoming messages and potentially adds them to the input buffer.
		 * @param messageID The ID of the received message.
		 */
		void OnMessageAccepted(Message::MessageID messageID) override;

		/**
		 * @brief Processes buffered messages to check if they trigger state changes or combos.
		 * @param deltaTime The time elapsed since the last frame.
		 */
		void ProcessBuffer(float deltaTime);

		/// @brief Checks if the actor is currently in an attacking state.
		bool IsAttacking() const;

		/// @brief Checks if the actor is currently in a blocking state.
		bool IsBlocking() const;
		
		const std::string& GetCurrentStateName() const { return m_currentStateName; }
		const std::unordered_map<std::string, StateConfig>& GetConfigs() const { return m_stateConfigs; }

		// --- Modular Asset Configuration ---
		const std::string& GetConfigPath() const { return m_configPath; }
		void SetConfigPath(const std::string& path) { m_configPath = path; }

		/**
		 * @brief Loads states, combos, and transitions from a standalone combat JSON asset.
		 * @param path Relative path to the combat asset file.
		 * @return True if loaded successfully, false on I/O or parsing error.
		 */
		bool LoadFromConfigFile(const std::string& path);

		/**
		 * @brief Persists the current state configurations out to a standalone combat JSON asset.
		 * @param path Target file path. If empty, uses m_configPath.
		 * @return True if written successfully, false on error.
		 */
		bool SaveToConfigFile(const std::string& path = "") const;

	private:
		/**
		 * @brief Internal helper to parse states array and defaultState from a JSON object.
		 */
		void DeserializeStatesFromJSON(const nlohmann::json& data);

		/**
		 * @brief Callback triggered when an overlap event occurs (e.g. hit detection).
		 * @param payLoad Data containing information about the overlap event.
		 */
		void OnTriggerOverLap(const HEIN::TriggerEventPayLoad& payLoad);

		void RebuildAllStates();
	};
}

