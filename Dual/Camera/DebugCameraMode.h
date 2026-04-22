#pragma once
#include <pch.h>
#include "ICameraMode.h"

class DebugCameraMode : public ICameraMode
{
private:
	 
	DirectX::SimpleMath::Vector3 m_target;
	float m_yaw;
	float m_pitch;
	float m_distance;
	float m_mouseSensitivity;

public:

	DebugCameraMode(float startDistance = 5.0f, DirectX::SimpleMath::Vector3 target = DirectX::SimpleMath::Vector3::Zero)
		: m_target(target)
		, m_yaw(0.0f)
		, m_pitch(0.0f)
		, m_distance(startDistance)
		, m_mouseSensitivity(0.09f)
	{
	}

	void ProcessInput(const CameraInputState& input) override 
	{
		m_yaw += input.mouseX * m_mouseSensitivity;
		m_pitch += input.mouseY * m_mouseSensitivity;

		const float pitchLimit = DirectX::XM_PIDIV2 - 0.01f;
		m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

		if (input.scrollWheelDelta != 0.0f)
		{
			m_distance -= (input.scrollWheelDelta / 100.0f);
			m_distance = std::max(0.1f, m_distance);
		}

	}

	void Update(CameraData& outData, float deltaTime) override
	{
		DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);

		DirectX::SimpleMath::Vector3 offset = rotation.Backward() * m_distance;

		outData.position = m_target + offset;
		outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(outData.position, m_target, DirectX::SimpleMath::Vector3::Up);
		outData.fov = DirectX::XM_PIDIV4;

	}
};
