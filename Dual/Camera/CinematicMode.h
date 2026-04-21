#pragma once
#include "ICameraMode.h"

class CinematicMode : public ICameraMode
{
private:

	DirectX::SimpleMath::Vector3 m_centerofStage;
	float m_radius;
	float m_currentAngle;
	float m_orbitSpeed;
	float m_cameraHeight;

public:

	CinematicMode(DirectX::SimpleMath::Vector3 centerPoint, float radius = 8.0f, float currentAngle = 0.0f, float orbitSpeed = 0.5f, float cameraHeight = 2.5f)
		: m_centerofStage(centerPoint)
		, m_radius(radius)
		, m_currentAngle(currentAngle)
		, m_orbitSpeed(orbitSpeed)
		, m_cameraHeight(cameraHeight)

	{
	}

	// Cinematic mode ignores player input
	void ProcessInput(const CameraInputState& input) override { return; }

	void Update(CameraData& outData, float deltaTime) override
	{
		// Advance the angle over time
		m_currentAngle += m_orbitSpeed * deltaTime;

		// Calculate the camera position by trigonometry for a circle

		float x = m_centerofStage.x + (cosf(m_currentAngle) * m_radius);
		float z = m_centerofStage.z + (sinf(m_currentAngle) * m_radius);

		outData.position = DirectX::SimpleMath::Vector3(x, m_centerofStage.y + m_cameraHeight, z);

		// Always look perfectly at the center of the stage 
		outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(
			outData.position,
			m_centerofStage,
			DirectX::SimpleMath::Vector3::Up
		);

		// FOV
		outData.fov = DirectX::XM_PIDIV4;
	}
};

