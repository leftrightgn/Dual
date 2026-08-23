#include "pch.h"
#include "PlayerInputComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "../../External/Engine/Camera/CameraController.h"
#include "../../../External/Engine/Entities/ActorManager.h"
#include "../../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Message/Messenger.h" 
#include "../../External/Engine/Message/Message.h"  
#include "../../External/Engine/Framework/GameContext.h"
#include <Components/CombatStateMachineComponent.h>


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
	Messenger::GetInstance()->Register(m_owner->GetID(), this);
}

void HEIN::PlayerInputComponent::ProcessInput(const GameContext& gameContext)
{
	if (m_actorManager == nullptr || m_blackboard == nullptr) return;
	HEIN::CameraController* cameraController = gameContext.mainCamera;

	if (m_blackboard != nullptr)
	{
		// Get logical movement directly from the manager
		DirectX::SimpleMath::Vector3 localInput = m_localInput;
		DirectX::SimpleMath::Vector3 worldIntent = DirectX::SimpleMath::Vector3::Zero;
		float cameraYaw = 0.0f;

		if (cameraController != nullptr)
		{
			// Transform local player movement into world space relative to the camera's facing direction
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

		bool isCameraLocked = false;
        if (cameraController != nullptr && cameraController->GetCurrentCameraType() == HEIN::CameraType::LockOn)
        {
            isCameraLocked = true;
        }
        
        m_blackboard->isLockedOn = isCameraLocked;

		m_blackboard->localMoveIntent = localInput;

		m_localInput = DirectX::SimpleMath::Vector3::Zero;
	}
}

void HEIN::PlayerInputComponent::OnMessageAccepted(Message::MessageID messageID)
{
	// Accumulate local directional inputs from messages
	switch (messageID)
	{
	case Message::PLAYER_MOVE_FORWARD:  m_localInput.z += 1.0f; break;
	case Message::PLAYER_MOVE_BACKWARD: m_localInput.z -= 1.0f; break;
	case Message::PLAYER_MOVE_LEFT:     m_localInput.x += 1.0f; break;
	case Message::PLAYER_MOVE_RIGHT:    m_localInput.x -= 1.0f; break;
	case Message::PLAYER_STOP_MOVEMENT: m_localInput = DirectX::SimpleMath::Vector3::Zero; break;
	}

	HEIN::CombatStateMachineComponent* stateMachine = m_owner->GetComponent<HEIN::CombatStateMachineComponent>();
	if (stateMachine != nullptr)
	{
		stateMachine->OnMessageAccepted(messageID);
	}
}
