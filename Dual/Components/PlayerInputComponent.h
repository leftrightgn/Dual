#pragma once
#include "Components/IComponent.h"

namespace HEIN
{
	struct CombatBlackBoard;
	class Actor;
	class CameraController;

	class PlayerInputComponent : public HEIN::IComponent
	{
	private:

		static constexpr float NETUAL_PITCH = 0.0f;
		static constexpr float NETUAL_ROLL = 0.0f;

	private:

		HEIN::CombatBlackBoard* m_blackboard = nullptr;
		HEIN::CameraController* m_cameraController = nullptr; 

	public:

		PlayerInputComponent(Actor* owner, HEIN::CameraController* cameraController);

		void Start();

		void ProcessInput(const GameContext& gameContext);

		void Update(float deltaTime) override;
	};
}