#include "pch.h"
#include "FirstPersonMode.h"

HEIN::FirstPersonMode::FirstPersonMode(
	  const DirectX::SimpleMath::Vector3* headPos,
	  SkinnedModelComponent* fpsModel,
	  SkinnedModelComponent* tpsModel
)
	: m_playerHeadPosition(headPos)
	, m_fpsModel(fpsModel)
	, m_tpsModel(tpsModel)
	, m_pitch(PITCH)
	, m_yaw(YAW)
	, m_mouseSensitivity(MOUSE_SENSITIVITY)
	, m_targetHeight(TARGET_HEIGHT)
	, m_boomlenght(BOOM_LENGTH)
{
}

void HEIN::FirstPersonMode::OnEnter(CameraData& /*data*/)
{
	if (m_fpsModel != nullptr) m_fpsModel->SetVisible(true);
	if (m_tpsModel != nullptr) m_tpsModel->SetVisible(false);
}

void HEIN::FirstPersonMode::ProcessInput(const CameraInputState& input)
{
	m_yaw += -input.mouseX * m_mouseSensitivity;
	m_pitch += -input.mouseY * m_mouseSensitivity;


	constexpr float pitchLimit = (DirectX::XMConvertToRadians(80.0f));
	m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

	/*constexpr float yawLimit = (DirectX::XMConvertToRadians(60.0f));
	m_yaw = std::clamp(m_yaw, -yawLimit, yawLimit);*/
}

void HEIN::FirstPersonMode::Update(CameraData& outData, float /*deltaTime*/, ICameraController& /*controller*/)
{
	outData.position = *m_playerHeadPosition;
	outData.position.y += m_targetHeight;

	DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);

	DirectX::SimpleMath::Matrix yawOnly = DirectX::SimpleMath::Matrix::CreateRotationY(m_yaw);
	DirectX::SimpleMath::Vector3 flatForward = yawOnly.Forward();
	DirectX::SimpleMath::Vector3 right = yawOnly.Right();

	flatForward.Normalize();

	outData.position += flatForward * m_boomlenght;
	outData.position += right * -0.3f;

	DirectX::SimpleMath::Vector3 target = outData.position + rotation.Forward();
	DirectX::SimpleMath::Vector3 up = DirectX::SimpleMath::Vector3::Up;

	outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(
		outData.position,
		target,
		up
	);

	outData.fov = DirectX::XMConvertToRadians(90.0f);
}