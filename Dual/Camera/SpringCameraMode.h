#pragma once
#include "ICameraMode.h"
#include <SimpleMath.h>

namespace HEIN
{

	class SpringCameraMode : public ICameraMode
	{
	private:

		const DirectX::SimpleMath::Vector3* m_desiredEye;
		const DirectX::SimpleMath::Vector3* m_desiredTarget;
		DirectX::SimpleMath::Vector3 m_currentPosition;
		DirectX::SimpleMath::Vector3 m_currentLookAt;
		DirectX::SimpleMath::Vector3 m_positionVelocity;
		DirectX::SimpleMath::Vector3 m_lookAtVelocity;
		float m_stiffness;
		float m_damping;

	public:

		SpringCameraMode(const DirectX::SimpleMath::Vector3* desiredEye, const DirectX::SimpleMath::Vector3* desiredTarget, float freq = 8.0f)
			: m_desiredEye(desiredEye)
			, m_desiredTarget(desiredTarget)
			, m_positionVelocity(DirectX::SimpleMath::Vector3::Zero)
			, m_lookAtVelocity(DirectX::SimpleMath::Vector3::Zero)

		{
			SetFrequency(freq);
		}

		void ProcessInput(const CameraInputState& /*input*/) override { return; }

		void Update(CameraData& outData, float deltaTime, ICameraController& /*controller*/) override
		{
			UpdateSpring(*m_desiredEye, m_currentPosition, m_positionVelocity, deltaTime);
			UpdateSpring(*m_desiredTarget, m_currentLookAt, m_lookAtVelocity, deltaTime);

			outData.position = m_currentPosition;
			outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(outData.position, m_currentLookAt, DirectX::SimpleMath::Vector3::Up);
			outData.fov = DirectX::XMConvertToRadians(50.0f);
		}

		void SetFrequency(float freq)
		{
			m_stiffness = freq * freq;
			m_damping = 2.0f * freq;
		}

	private:

		void UpdateSpring(const DirectX::SimpleMath::Vector3& target, DirectX::SimpleMath::Vector3& current, DirectX::SimpleMath::Vector3& velocity, float elapsedTime) const
		{
			DirectX::SimpleMath::Vector3 delta = target - current;
			DirectX::SimpleMath::Vector3 accel = (m_stiffness * delta) - (m_damping * velocity);
			velocity += accel * elapsedTime;
			current += velocity * elapsedTime;
		}

	};
}
