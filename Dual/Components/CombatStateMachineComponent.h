#pragma once
#include "../../External/Engine/Common/Event.h"
#include "../../External/Engine/Components/IComponent.h"
#include "../../External/Engine/Message/IObserver.h"
#include "../../External/Engine/Message/Message.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

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

		/// @brief A map of all available states, keyed by their string name.
		std::unordered_map<std::string, std::unique_ptr<ICombatState>> m_states;

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

	public:
		CombatStateMachineComponent(Actor* owner);
	
		void Start() override;

		void Update(float deltaTime) override;

		/**
		 * @brief Requests a transition to a new combat state.
		 * @param stateName The string identifier for the target state.
		 */
		void ChangeState(const std::string& stateName);

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
		void AddState(const std::string& stateName, std::unique_ptr<ICombatState> state);

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
		
	private:
		/**
		 * @brief Callback triggered when an overlap event occurs (e.g. hit detection).
		 * @param payLoad Data containing information about the overlap event.
		 */
		void OnTriggerOverLap(const HEIN::TriggerEventPayLoad& payLoad);
	};
}
