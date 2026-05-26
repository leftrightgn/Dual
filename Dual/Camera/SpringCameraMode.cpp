#include "pch.h"
#include "SpringCameraMode.h"
#include "Components/TransformComponent.h"


HEIN::SpringCameraMode::SpringCameraMode(
	  const TransformComponent* targetTransform,
	  const DirectX::SimpleMath::Vector3* desiredTarget, 
	  float followDistance, 
	  float heightOffset, 
	  float freq
)
	: m_targetTransform(targetTransform)
	, m_desiredTarget(desiredTarget)
	, m_currentPosition(DirectX::SimpleMath::Vector3::Zero)
	, m_currentLookAt(DirectX::SimpleMath::Vector3::Zero)
	, m_positionVelocity(DirectX::SimpleMath::Vector3::Zero)
	, m_lookAtVelocity(DirectX::SimpleMath::Vector3::Zero)
	, m_pitch(PITCH)
	, m_yaw(YAW)
	, m_mouseSensitivity(DEFAULT_MOUSE_SENSITIVITY)
	, m_followDistance(followDistance)
	, m_heightOffset(heightOffset)
	, m_isInitialized(false)
{
	SetFrequency(freq);
}

void HEIN::SpringCameraMode::ProcessInput(const CameraInputState& input)
{
	m_yaw += -input.mouseX * m_mouseSensitivity;
	m_pitch += -input.mouseY * m_mouseSensitivity;

	constexpr float maxPitchDown = (DirectX::XMConvertToRadians(5.0f));  // look down
	constexpr float maxPitchUp = -(DirectX::XMConvertToRadians(45.0f));  // look up

	// clamp the pitch 
	m_pitch = std::clamp(m_pitch, maxPitchUp, maxPitchDown);
}

void HEIN::SpringCameraMode::Update(CameraData& outData, float deltaTime, ICameraController& /*controller*/)
{
	if (!m_targetTransform || !m_desiredTarget) return;

	DirectX::SimpleMath::Vector3 targetLookAt = *m_desiredTarget;

	DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
	DirectX::SimpleMath::Vector3 shoulderOffset = rotation.Right() * 0.5f;

	DirectX::SimpleMath::Vector3 camBackWard = rotation.Backward();
	

	DirectX::SimpleMath::Vector3 targetEye = targetLookAt + (camBackWard * m_followDistance) + DirectX::SimpleMath::Vector3(0.0f, m_heightOffset, 0.0f) + shoulderOffset;

	if (!m_isInitialized)
	{
		m_currentPosition = targetEye;
		m_currentLookAt = targetLookAt;
		m_isInitialized = true;
	}

	UpdateSpring(targetEye, m_currentPosition, m_positionVelocity, deltaTime);
	UpdateSpring(targetLookAt, m_currentLookAt, m_lookAtVelocity, deltaTime);

	outData.position = m_currentPosition;

	outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(outData.position, m_currentLookAt, DirectX::SimpleMath::Vector3::Up);

	outData.fov = DirectX::XMConvertToRadians(50.0f);

}

void HEIN::SpringCameraMode::SetFrequency(float freq)
{
	m_stiffness = freq * freq;
	m_damping = 2.0f * freq;
}

void HEIN::SpringCameraMode::UpdateSpring(
	const DirectX::SimpleMath::Vector3& target,
	DirectX::SimpleMath::Vector3& current,
	DirectX::SimpleMath::Vector3& velocity,
	float elapsedTime
) const
	{
		DirectX::SimpleMath::Vector3 delta = target - current;
		DirectX::SimpleMath::Vector3 accel = (m_stiffness * delta) - (m_damping * velocity);
		velocity += accel * elapsedTime;
		current += velocity * elapsedTime;
	}
