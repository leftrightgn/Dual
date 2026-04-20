#pragma once
#include "ICameraMode.h"
#include <algorithm>


class ThirdPersonMode : public ICameraMode
{
private:

	const DirectX::SimpleMath::Vector3* m_playerTarget;
	float m_pitch;
	float m_yaw;
	float m_boomLength;
	float m_mouseSensitivity;

public:

	ThirdPersonMode(DirectX::SimpleMath::Vector3* playerTarget)
	  	: m_playerTarget(playerTarget)
		, m_pitch(0.0f)
		, m_yaw(0.0f)
		, m_boomLength(3.0f)
		, m_mouseSensitivity(0.004f)
	{
	}

	void ProcessInput(float mouseX, float mouseY) override
	{
		m_pitch += mouseX * m_mouseSensitivity;
		m_yaw += mouseY * m_mouseSensitivity;

		// max look down 85degree
		const float maxPitchDown = (DirectX::XM_PIDIV2 - 1.0f);

		// max look up 45degree
		const float maxPitchUp = -(DirectX::XM_PIDIV4);

		// clamp the pitch 
		m_pitch = std::clamp(m_pitch, maxPitchUp, maxPitchDown);
	}

	void Update(CameraData& outData, float deltaTime) override
	{
		DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_pitch, m_yaw, 0.0f);
		DirectX::SimpleMath::Vector3 offset = rotation.Backward() * m_boomLength;
		DirectX::SimpleMath::Vector3 shoulderOffset = rotation.Right() * 0.5f;

		outData.position = *m_playerTarget + offset + shoulderOffset;
		outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(
			outData.position,
			*m_playerTarget,
			DirectX::SimpleMath::Vector3::Up
		);

		outData.fov = DirectX::XM_PI / 3.0f;
	}

};
