#include "pch.h"
#include "PlayerInputComponent.h"
#include "TransformComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Camera/CameraController.h"
#include "Entities/Actor.h"
#include <Mouse.h>
#include <Keyboard.h>
#include "../GameContext.h"

HEIN::PlayerInputComponent::PlayerInputComponent(Actor* owner, HEIN::CameraController* cameraController)
	: HEIN::IComponent(owner)
	, m_cameraController(cameraController)
{
}

void HEIN::PlayerInputComponent::Start()
{
	m_blackboard = m_owner->GetComponent<HEIN::CombatBlackBoard>();
}

void HEIN::PlayerInputComponent::ProcessInput(const GameContext& gameContext)
{
	// Capture member into a local variable so the static analyzer can see the null-check covers all uses
	HEIN::CameraController* cameraController = m_cameraController;

	if (cameraController != nullptr)
	{
		HEIN::CameraInputState cameraInput;
		DirectX::Mouse::State mouseState = gameContext.mouseState;

		cameraInput.mouseX = static_cast<float>(mouseState.x);
		cameraInput.mouseY = static_cast<float>(mouseState.y);
		cameraInput.isLeftMouseDown = mouseState.leftButton;
		cameraInput.scrollWheelDelta = static_cast<float>(mouseState.scrollWheelValue);

		cameraController->ProcessInput(cameraInput);

		// Handle Camera Switching
		if (gameContext.keyboardTracker.pressed.T)
		{
			cameraController->RequestSwitch(HEIN::CameraType::ThirdPerson);
		}
		if (gameContext.keyboardTracker.pressed.P)
		{
			cameraController->RequestSwitch(HEIN::CameraType::FirstPerson);
		}
		if (gameContext.keyboardTracker.pressed.E)
		{
			cameraController->RequestSwitch(HEIN::CameraType::Spring);
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

		DirectX::SimpleMath::Vector3 worldIntent = DirectX::SimpleMath::Vector3::Zero;

		// If we have a camera controller, transform input by camera yaw.
		if (cameraController != nullptr)
		{
			// Get the Camera's Forward direction
			DirectX::SimpleMath::Matrix view = cameraController->GetView();
			DirectX::SimpleMath::Matrix invView = view.Invert();
			DirectX::SimpleMath::Vector3 camForward = invView.Forward();

			// Calculate the pure Camera Yaw
			float cameraYaw = atan2f(camForward.x, camForward.z);

			// Transform local WASD input into World Direction based on the Camera
			DirectX::SimpleMath::Matrix camRotation = DirectX::SimpleMath::Matrix::CreateRotationY(cameraYaw);
			worldIntent = DirectX::SimpleMath::Vector3::TransformNormal(localInput, camRotation);

			if (cameraController->LocksPlayerRotation())
			{
				HEIN::TransformComponent* transform = m_owner->GetComponent<HEIN::TransformComponent>();
				if (transform)
				{
					float YAW = cameraYaw + DirectX::XM_PI;
					DirectX::SimpleMath::Quaternion alignedRot =
						DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll
						(
							YAW,
							NETUAL_PITCH,
							NETUAL_ROLL
						);

					transform->SetRotation(alignedRot);
				}
			}
		}
		else
		{
			// No camera: interpret local input directly as world intent (no rotation)
			worldIntent = localInput;
		}

		if (worldIntent.LengthSquared() > 0) worldIntent.Normalize();
		m_blackboard->moveIntent = worldIntent; // intent is in absolute world space!

		m_blackboard->isAttackingIntent = gameContext.mouseState.leftButton;
		m_blackboard->isParryingIntent = gameContext.mouseState.rightButton;
	}
}

void HEIN::PlayerInputComponent::Update(float /*deltaTime*/)
{
}
