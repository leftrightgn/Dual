#pragma once
#include "Components/IComponent.h"
#include <Message/IObserver.h>

namespace HEIN
{
	struct CombatBlackBoard;
	class Actor;
	class ActorManager;

	class PlayerInputComponent : public HEIN::IComponent, public IObserver
	{
	private:

		static constexpr float NETUAL_PITCH = 0.0f;
		static constexpr float NETUAL_ROLL = 0.0f;

	private:

		HEIN::CombatBlackBoard* m_blackboard = nullptr;
		HEIN::ActorManager* m_actorManager = nullptr;

		DirectX::SimpleMath::Vector3 m_localInput = DirectX::SimpleMath::Vector3::Zero;

	public:

		PlayerInputComponent(
			Actor* owner, 
			ActorManager* actorManager
		);

		void Start() override;

		void ProcessInput(const GameContext& gameContext);

		void Update(float deltaTime) override {}

		void OnMessageAccepted(Message::MessageID messageID) override;
	};
}