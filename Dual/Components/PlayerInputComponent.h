#pragma once
#include "Components/IComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Camera/CameraController.h"
#include "GameContext.h"
#include "Entities/Actor.h"
#include <Mouse.h>


class PlayerInputComponent : public HEIN::IComponent
{
private:

	HEIN::CombatBlackBoard* m_blackboard = nullptr;
	HEIN::CameraController* m_cameraController = nullptr;

public:

	PlayerInputComponent(Actor* owner, HEIN::CameraController* cameraController)
		: HEIN::IComponent(owner)
		, m_cameraController(cameraController)
	{
	}
	
	void Start()
	{
		m_blackboard = m_owner->GetComponent<HEIN::CombatBlackBoard>();
	}

	void ProcessInput(const GameContext& gameContext)
	{
		if (m_cameraController != nullptr)
		{
			HEIN::CameraInputState cameraInput;
			DirectX::Mouse::State mouseState = gameContext.mouseState;

			cameraInput.mouseX = static_cast<float>(mouseState.x);
			cameraInput.mouseY = static_cast<float>(mouseState.y);
			cameraInput.isLeftMouseDown = mouseState.leftButton;
			cameraInput.scrollWheelDelta = static_cast<float>(mouseState.scrollWheelValue);

			m_cameraController->ProcessInput(cameraInput);

			// Handle Camera Switching
			if (gameContext.keyboardTracker.pressed.T)
			{
				m_cameraController->RequestSwitch(HEIN::CameraType::ThirdPerson);
			}
			if (gameContext.keyboardTracker.pressed.P)
			{
				m_cameraController->RequestSwitch(HEIN::CameraType::FirstPerson);
			}
		}

		if (m_blackboard != nullptr)
		{
			DirectX::SimpleMath::Vector3 inputDir = DirectX::SimpleMath::Vector3::Zero;

			if (gameContext.keyboardTracker.pressed.W) inputDir.z += 1.0f;
			if (gameContext.keyboardTracker.pressed.S) inputDir.z -= 1.0f;
			if (gameContext.keyboardTracker.pressed.A) inputDir.x -= 1.0f;
			if (gameContext.keyboardTracker.pressed.D) inputDir.x += 1.0f;


			if (inputDir.LengthSquared() > 0) inputDir.Normalize();
			m_blackboard->moveIntent = inputDir;

			m_blackboard->isAttackingIntent = gameContext.mouseState.leftButton;
			m_blackboard->isParryingIntent = gameContext.mouseState.rightButton;
		}
	}

	void Update(float deltaTime) override {}
};
