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
	, m_followDistance(followDistance)
	, m_heightOffset(heightOffset)
	, m_isInitialized(false)
{
	SetFrequency(freq);
}

void HEIN::SpringCameraMode::Update(CameraData& outData, float deltaTime, ICameraController& /*controller*/)
{
	if (!m_targetTransform || !m_desiredTarget) return;

	DirectX::SimpleMath::Vector3 targetLookAt = *m_desiredTarget;

	DirectX::SimpleMath::Matrix worldMatrix = m_targetTransform->GetWorldMatrix();
	DirectX::SimpleMath::Vector3 backward = worldMatrix.Backward();

	backward.y = 0.0f;
	backward.Normalize();

	DirectX::SimpleMath::Vector3 targetEye = targetLookAt + (backward * m_followDistance) + DirectX::SimpleMath::Vector3(0.0f, m_heightOffset, 0.0f);

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
