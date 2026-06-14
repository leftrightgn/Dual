#include "pch.h"
#include "PlayerInputComponent.h"
#include "TransformComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Camera/CameraController.h"
#include "Entities/Actor.h"
#include <Mouse.h>
#include <Keyboard.h>
#include "Framework/GameContext.h"

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
		const DirectX::Mouse::State& mouseState = gameContext.mouseState;

		cameraInput.mouseX = static_cast<float>(mouseState.x);
		cameraInput.mouseY = static_cast<float>(mouseState.y);
		cameraInput.isLeftMouseDown = mouseState.leftButton;
		cameraInput.scrollWheelDelta = static_cast<float>(mouseState.scrollWheelValue);

		cameraController->ProcessInput(cameraInput);

		// Handle Camera Switching cleanly
		HEIN::CameraType targetCameraType;
		if (gameContext.inputManager->WasCameraSwitchPressed(gameContext, targetCameraType))
		{
			cameraController->RequestSwitch(targetCameraType);
		}
	}

	if (m_blackboard != nullptr)
	{
		// Get logical movement directly from the manager
		DirectX::SimpleMath::Vector3 localInput = gameContext.inputManager->GetMoveIntent(gameContext);
		DirectX::SimpleMath::Vector3 worldIntent = DirectX::SimpleMath::Vector3::Zero;
		float cameraYaw = 0.0f;

		if (cameraController != nullptr)
		{
			DirectX::SimpleMath::Matrix view = cameraController->GetView();
			DirectX::SimpleMath::Matrix invView = view.Invert();
			DirectX::SimpleMath::Vector3 camForward = invView.Forward();

			float cameraYaw = atan2f(camForward.x, camForward.z);
			DirectX::SimpleMath::Matrix camRotation = DirectX::SimpleMath::Matrix::CreateRotationY(cameraYaw);
			worldIntent = DirectX::SimpleMath::Vector3::TransformNormal(localInput, camRotation);

		}
		else
		{
			worldIntent = localInput;
		}
		
		if (worldIntent.LengthSquared() > 0) worldIntent.Normalize();
		m_blackboard->moveIntent = worldIntent;

		// Get combat logic cleanly
		m_blackboard->isAttackingIntent = gameContext.inputManager->IsAttacking(gameContext);
	}
}

void HEIN::PlayerInputComponent::Update(float deltaTime)
{
	if (m_owner == nullptr || m_blackboard == nullptr) return;

	HEIN::TransformComponent* transform = m_owner->GetComponent<HEIN::TransformComponent>();
	if (transform != nullptr)
	{
		DirectX::SimpleMath::Quaternion currentRot = transform->GetRotation();
		DirectX::SimpleMath::Quaternion targetRot = currentRot;
		bool isRotating = false;
		float slerpSpeed = 10.0f;

		if (m_cameraController != nullptr && m_cameraController->LocksPlayerRotation())
		{
			// FPS / STRAFING (Locked to Camera)
			DirectX::SimpleMath::Matrix view = m_cameraController->GetView();
			DirectX::SimpleMath::Matrix invView = view.Invert();
			DirectX::SimpleMath::Vector3 camForward = invView.Forward();

			float cameraYaw = atan2f(camForward.x, camForward.z);
			float lockedYaw = cameraYaw + DirectX::XM_PI;

			targetRot = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(lockedYaw, NETUAL_PITCH, NETUAL_ROLL);
			isRotating = true;
			slerpSpeed = 25.0f; // Very fast Slerp so the crosshair stays highly responsive
		}
		else if (m_blackboard->moveIntent.LengthSquared() > 0.0f)
		{
			// Free Movement
			float freeYaw = atan2f(m_blackboard->moveIntent.x, m_blackboard->moveIntent.z) + DirectX::XM_PI;

			targetRot = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(freeYaw, NETUAL_PITCH, NETUAL_ROLL);
			isRotating = true;
			slerpSpeed = 5.0f; // Slower
		}

		// Apply the Slerp Math
		if (isRotating)
		{
			// Cap the blend factor at 1.0f to prevent math crashes during lag spikes
			float t = deltaTime * slerpSpeed;
			if (t > 1.0f) t = 1.0f;

			// Slerp calculates the perfectly smooth spherical curve between the current and target rotation
			DirectX::SimpleMath::Quaternion blendedRot = DirectX::SimpleMath::Quaternion::Slerp(currentRot, targetRot, t);

			transform->SetRotation(blendedRot);
		}
	}
}
