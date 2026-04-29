#pragma once
#include "Components/IComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Camera/CameraController.h"
#include "GameContext.h"
#include "Entities/Actor.h"
#include <Mouse.h>
#include <Keyboard.h>


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
			DirectX::SimpleMath::Vector3 localInput = DirectX::SimpleMath::Vector3::Zero;

			DirectX::Keyboard::State kbState = DirectX::Keyboard::Get().GetState();

			if (kbState.W) localInput.z += 1.0f;
			if (kbState.S) localInput.z -= 1.0f;
			if (kbState.A) localInput.x += 1.0f;
			if (kbState.D) localInput.x -= 1.0f;

			// Get the Camera's Forward direction
			DirectX::SimpleMath::Matrix view = m_cameraController->GetView();
			DirectX::SimpleMath::Matrix invView = view.Invert();
			DirectX::SimpleMath::Vector3 camForward = invView.Forward();

			// Calculate the pure Camera Yaw
			float cameraYaw = atan2f(camForward.x, camForward.z);

			// Transform local WASD input into World Direction based on the Camera
			DirectX::SimpleMath::Matrix camRotation = DirectX::SimpleMath::Matrix::CreateRotationY(cameraYaw);
			DirectX::SimpleMath::Vector3 worldIntent = DirectX::SimpleMath::Vector3::TransformNormal(localInput, camRotation);

			if (worldIntent.LengthSquared() > 0) worldIntent.Normalize();
			m_blackboard->moveIntent = worldIntent; // intent is in absolute world space!

			// Force the character to always face the camera's target 
			HEIN::TransformComponent* transform = m_owner->GetComponent<HEIN::TransformComponent>();
			if (transform)
			{
				DirectX::SimpleMath::Vector3 rot = transform->GetRotation();
				rot.y = cameraYaw + DirectX::XM_PI;
				transform->SetRotation(rot);
			}

			m_blackboard->isAttackingIntent = gameContext.mouseState.leftButton;
			m_blackboard->isParryingIntent = gameContext.mouseState.rightButton;
		}
	}

	void Update(float deltaTime) override {}
};
