#include "pch.h"
#include "ThirdPersonMode.h"

HEIN::ThirdPersonMode::ThirdPersonMode(
	  DirectX::SimpleMath::Vector3* playerTarget,
	  SkinnedModelComponent* fpsModel,
	  SkinnedModelComponent* tpsModel
)
	: m_playerTarget(playerTarget)
	, m_fpsModel(fpsModel)
	, m_tpsModel(tpsModel)
	, m_pitch(PITCH)
	, m_yaw(YAW)
	, m_boomLength(BOOM_LENGTH)
	, m_mouseSensitivity(MOUSE_SENSITIVITY)
	, m_targetHeight(TARGET_HEIGHT)
{
}

void HEIN::ThirdPersonMode::OnEnter(CameraData& data)
{
	if (m_fpsModel != nullptr) m_fpsModel->SetVisible(false);
	if (m_tpsModel != nullptr) m_tpsModel->SetVisible(true);
}

void HEIN::ThirdPersonMode::ProcessInput(const CameraInputState& input)
{
	m_yaw += -input.mouseX * m_mouseSensitivity;
	m_pitch += -input.mouseY * m_mouseSensitivity;

	constexpr float maxPitchDown = (DirectX::XMConvertToRadians(5.0f));  // look down
	constexpr float maxPitchUp = -(DirectX::XMConvertToRadians(45.0f));  // look up

	// clamp the pitch 
	m_pitch = std::clamp(m_pitch, maxPitchUp, maxPitchDown);

}

void HEIN::ThirdPersonMode::Update(CameraData& outData, float /*deltaTime*/, ICameraController& /*controller*/)
{
	DirectX::SimpleMath::Vector3 focalPoint = *m_playerTarget;
	focalPoint.y += m_targetHeight;

	DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
	DirectX::SimpleMath::Vector3 offset = rotation.Backward() * m_boomLength;
	DirectX::SimpleMath::Vector3 shoulderOffset = rotation.Right() * 0.5f;

	outData.position = focalPoint + offset + shoulderOffset;
	outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(
		outData.position,
		focalPoint,
		DirectX::SimpleMath::Vector3::Up
	);

	outData.fov = DirectX::XMConvertToRadians(50.0f);
}
