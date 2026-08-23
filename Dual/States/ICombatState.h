#pragma once
#include "../../../External/Engine/Message/Message.h"
namespace HEIN
{
	class CombatStateMachineComponent;
	class Actor;

	/**
	 * @class ICombatState
	 * @brief Base interface for individual states within the combat state machine.
	 * 
	 * Defines the standard lifecycle (OnEnter, Update, OnExit) and message handling
	 * for combat states like attacking, dodging, or blocking. Enables dynamic state
	 * transitions and animation crossfading.
	 */
	class ICombatState
	{
	public:

		virtual ~ICombatState() = default;

		/**
		 * @brief Called when the state is entered.
		 * @param owner The actor that owns the state machine.
		 * @param stateMachine The state machine component managing this state.
		 */
		virtual void OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine) = 0;

		/**
		 * @brief Called every frame to update the state.
		 * @param owner The actor that owns the state machine.
		 * @param stateMachine The state machine component managing this state.
		 * @param deltaTime The time elapsed since the last frame.
		 */
		virtual void Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime) = 0;

		/**
		 * @brief Called when the state is exited.
		 * @param owner The actor that owns the state machine.
		 * @param stateMachine The state machine component managing this state.
		 */
		virtual void OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) = 0;

		/**
		 * @brief Handles incoming messages or events.
		 * @param owner The actor that owns the state machine.
		 * @param stateMachine The state machine component managing this state.
		 * @param messageID The ID of the message to handle.
		 * @return True if the message was handled, false otherwise.
		 */
		virtual bool HandleMessage(
			Actor* /*owner*/,
			CombatStateMachineComponent* /*stateMachine*/,
			Message::MessageID /*messageID*/
		)
		{ return false; }

		/**
		 * @brief Checks if this state is an attack state.
		 * @return True if it is an attack state, false otherwise.
		 */
		virtual bool IsAttackState() const { return false; }

		/**
		 * @brief Checks if this state is a block state.
		 * @return True if it is a block state, false otherwise.
		 */
		virtual bool IsBlockState() const { return false; }
	};
}
