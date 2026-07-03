#pragma once
#include "Components/IComponent.h"
#include <Entities/Actor.h>

namespace HEIN
{
	struct CombatBlackBoard;
	class Actor;
	class ActorManager;

	class PlayerInputComponent : public HEIN::IComponent
	{
	private:

		static constexpr float NETUAL_PITCH = 0.0f;
		static constexpr float NETUAL_ROLL = 0.0f;

	private:

		HEIN::CombatBlackBoard* m_blackboard = nullptr;
		HEIN::ActorManager* m_actorManager = nullptr;

	public:

		PlayerInputComponent(
			Actor* owner, 
			ActorManager* actorManager
		);

		void Start() override;

		void ProcessInput(const GameContext& gameContext);

		void Update(float deltaTime) override {}


	};
}