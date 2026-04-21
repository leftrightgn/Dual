#pragma once
#include "ICameraMode.h"
#include <algorithm>

class FirstPersonMode : public ICameraMode
{
private:

	const DirectX::SimpleMath::Vector3* m_playerHeadPosition;
	float m_pitch;
	float m_yaw;
	float m_mouseSensitivity;

public:
    
	FirstPersonMode(const DirectX::SimpleMath::Vector3* headPos)
		: m_playerHeadPosition(headPos)
		, m_pitch(0.0f)
		, m_yaw(0.0f)
		, m_mouseSensitivity(0.005f)
	{

	}

	void ProcessInput(const CameraInputState& input) override
	{
		m_yaw += input.mouseX * m_mouseSensitivity;
		m_pitch += input.mouseY * m_mouseSensitivity;

		// 85degree limit
		const float pitchLimit = (DirectX::XM_PIDIV2 - 0.1f);
		m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);
	}

	void Update(CameraData& outData, float deltaTime) override
	{
		// set the position to the playerhead
		outData.position = *m_playerHeadPosition;

		// calculate the rotation from accumulated pitch and yaw 
		DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);

		// Determine the forward and upward the vector based on that rotation
		DirectX::SimpleMath::Vector3 target = outData.position + rotation.Forward();
		DirectX::SimpleMath::Vector3 up = rotation.Up();

		// Create the final look at matrix
		outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(
			outData.position,
			target,
			up
		);

		// FOV
		outData.fov = DirectX::XM_PIDIV4;
	}
};

