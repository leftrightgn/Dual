#include "pch.h"
#include "PlayerInputComponent.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Entities/ActorManager.h"
#include "../../External/Engine/Camera/CameraController.h"
#include "../../External/Engine/Framework/GameContext.h"
#if HEIN_WITH_EDITOR
#include "../../External/Engine/ImGui/imgui.h"
#endif
#include "../../External/Engine/Message/Message.h"
#include "../../External/Engine/Message/Messenger.h"
#include "Components/CombatStateMachineComponent.h"

namespace HEIN
{
HEIN::PlayerInputComponent::PlayerInputComponent(
	Actor* owner, 
	ActorManager* actorManager
)
	: HEIN::IComponent(owner)
	, m_actorManager(actorManager)
{
}

HEIN::PlayerInputComponent::~PlayerInputComponent()
{
	if (Messenger* msg = Messenger::GetInstance())
	{
		msg->UnRegisterObserver(this);
	}
}

void HEIN::PlayerInputComponent::Start()
{
	m_blackboard = m_owner->GetComponent<HEIN::CombatBlackBoard>();
	if (Messenger* msg = Messenger::GetInstance())
	{
		msg->Register(m_owner->GetID(), this);
	}
}

void HEIN::PlayerInputComponent::ProcessInput(const GameContext& gameContext)
{
	m_blackboard = m_owner ? m_owner->GetComponent<HEIN::CombatBlackBoard>() : nullptr;
	if (m_actorManager == nullptr || m_blackboard == nullptr) return;
	HEIN::CameraController* cameraController = gameContext.mainCamera;

	if (m_blackboard != nullptr)
	{
		// Get logical movement directly from the manager
		DirectX::SimpleMath::Vector3 localInput = m_localInput;
		DirectX::SimpleMath::Vector3 worldIntent = DirectX::SimpleMath::Vector3::Zero;

		if (cameraController != nullptr)
		{
			// Project movement onto camera view plane (camera-relative movement)
			DirectX::SimpleMath::Matrix view = cameraController->GetView();
			DirectX::SimpleMath::Matrix invView = view.Invert();

			DirectX::SimpleMath::Vector3 camForward = invView.Forward();
			camForward.y = 0.0f;
			if (camForward.LengthSquared() > 0.0001f) camForward.Normalize();

			DirectX::SimpleMath::Vector3 camRight = invView.Right();
			camRight.y = 0.0f;
			if (camRight.LengthSquared() > 0.0001f) camRight.Normalize();

			worldIntent = (camForward * localInput.z) + (camRight * localInput.x);
		}
		else
		{
			worldIntent = localInput;
		}
		
		if (worldIntent.LengthSquared() > 0) worldIntent.Normalize();
		m_blackboard->moveIntent = worldIntent;

		bool isCameraLocked = false;
		if (m_owner && m_owner->GetActorType() != HEIN::ActorType::Ghost &&
			cameraController != nullptr && cameraController->GetCurrentCameraType() == HEIN::CameraType::LockOn)
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
	if (messageID == Message::PLAYER_TOGGLE_MODEL)
	{
		if (m_owner != nullptr)
		{
			m_owner->ToggleSkinnedModel();
		}
		return;
	}

	// Accumulate local directional inputs from messages
	switch (messageID)
	{
	case Message::PLAYER_MOVE_FORWARD:  m_localInput.z += 1.0f; break;
	case Message::PLAYER_MOVE_BACKWARD: m_localInput.z -= 1.0f; break;
	case Message::PLAYER_MOVE_LEFT:     m_localInput.x -= 1.0f; break;
	case Message::PLAYER_MOVE_RIGHT:    m_localInput.x += 1.0f; break;
	case Message::PLAYER_STOP_MOVEMENT: m_localInput = DirectX::SimpleMath::Vector3::Zero; break;
	}

	HEIN::CombatStateMachineComponent* stateMachine = m_owner->GetComponent<HEIN::CombatStateMachineComponent>();
	if (stateMachine != nullptr)
	{
		stateMachine->OnMessageAccepted(messageID);
	}
}

#if HEIN_WITH_EDITOR
void HEIN::PlayerInputComponent::OnInspectorGUI(GameContext& /*gameContext*/)
{
	if (ImGui::CollapsingHeader("PlayerInputComponent", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Input Mode: Active Player Controller");
		ImGui::Text("Blackboard: %s", m_blackboard ? "Connected" : "Disconnected");
		ImGui::Text("Local Intent: (%.2f, %.2f, %.2f)", m_localInput.x, m_localInput.y, m_localInput.z);
	}
}
#else
void HEIN::PlayerInputComponent::OnInspectorGUI(GameContext& /*gameContext*/) {}
#endif
} // namespace HEIN
