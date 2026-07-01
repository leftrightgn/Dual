#include "pch.h"
#include "PlayerInputComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "Camera/CameraController.h"
#include <Entities/ActorManager.h>
#include "Entities/Actor.h"
#include <Mouse.h>
#include "Framework/GameContext.h"
#include "IComponent.h"



HEIN::PlayerInputComponent::PlayerInputComponent(
	Actor* owner, 
	ActorManager* actorManager
)
	: HEIN::IComponent(owner)
	, m_actorManager(actorManager)
{
}

void HEIN::PlayerInputComponent::Start()
{
	m_blackboard = m_owner->GetComponent<HEIN::CombatBlackBoard>();
}

void HEIN::PlayerInputComponent::ProcessInput(const GameContext& gameContext)
{
	if (m_actorManager == nullptr || m_blackboard == nullptr) return;
	HEIN::CameraController* cameraController = gameContext.mainCamera;
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
		m_blackboard->isDodgingIntent = gameContext.inputManager->IsDodging(gameContext);
	}
}

void HEIN::PlayerInputComponent::Update(float deltaTime)
{
}
