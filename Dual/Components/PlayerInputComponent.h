#pragma once
#include "../../External/Engine/Components/IComponent.h"
#include "../../External/Engine/Message/IObserver.h"

namespace HEIN
{
	struct CombatBlackBoard;
	class Actor;
	class ActorManager;

	/**
	 * @class PlayerInputComponent
	 * @brief Translates user input into game commands and routes them via the blackboard.
	 *
	 * This component listens for raw input messages (keyboard, mouse, gamepad) and maps
	 * them to high-level game actions (e.g. movement vectors, attack signals) which it
	 * then writes to the CombatBlackBoard for other components to read.
	 */
	class PlayerInputComponent : public HEIN::IComponent, public IObserver
	{
	private:
		/// @brief Default pitch value used to keep the character upright.
		static constexpr float NETUAL_PITCH = 0.0f;
		
		/// @brief Default roll value used to keep the character upright.
		static constexpr float NETUAL_ROLL = 0.0f;

	private:
		/// @brief Pointer to the blackboard used to share state and input info across components.
		HEIN::CombatBlackBoard* m_blackboard = nullptr;
		
		/// @brief Pointer to the actor manager, used to fetch camera or other actor contexts.
		HEIN::ActorManager* m_actorManager = nullptr;

		/// @brief Represents the processed local movement input vector based on current keys down.
		DirectX::SimpleMath::Vector3 m_localInput = DirectX::SimpleMath::Vector3::Zero;

	public:
		PlayerInputComponent(
			Actor* owner, 
			ActorManager* actorManager
		);

		void Start() override;

		/**
		 * @brief Polls the input state and processes movement/camera logic based on the game context.
		 * @param gameContext Contains the global context including current inputs and time.
		 */
		void ProcessInput(const GameContext& gameContext);

		void Update(float /*deltaTime*/) override {}

		/**
		 * @brief Processes messages sent to the observer, converting them to game actions.
		 * @param messageID The incoming message ID to evaluate.
		 */
		void OnMessageAccepted(Message::MessageID messageID) override;
	};
}