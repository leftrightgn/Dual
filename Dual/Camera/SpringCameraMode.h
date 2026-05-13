#pragma once
#include "ICameraMode.h"
#include <SimpleMath.h>

namespace HEIN
{
	class TransformComponent;

	class SpringCameraMode : public ICameraMode
	{
	private:

		const TransformComponent* m_targetTransform;
		const DirectX::SimpleMath::Vector3* m_desiredTarget;
		DirectX::SimpleMath::Vector3 m_currentPosition;
		DirectX::SimpleMath::Vector3 m_currentLookAt;
		DirectX::SimpleMath::Vector3 m_positionVelocity;
		DirectX::SimpleMath::Vector3 m_lookAtVelocity;
		float m_stiffness;
		float m_damping;
		float m_followDistance;
		float m_heightOffset;
		bool  m_isInitialized;
	public:

		SpringCameraMode(
			const TransformComponent* targetTransform,
			const DirectX::SimpleMath::Vector3* desiredTarget,
			float followDistance = 4.5f,
			float heightOffset = 1.5f,
			float freq = 8.0f
		);
			
		void ProcessInput(const CameraInputState& /*input*/) override { return; }

		void Update(CameraData& outData, float deltaTime, ICameraController& controller) override;
		
		void SetFrequency(float freq);
	
	private:

		void UpdateSpring(
			const DirectX::SimpleMath::Vector3& target, 
			DirectX::SimpleMath::Vector3& current, 
			DirectX::SimpleMath::Vector3& velocity, 
			float elapsedTime) const;

	};
}
