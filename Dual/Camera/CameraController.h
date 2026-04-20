#pragma once
#include "ICameraMode.h"

class CameraController
{
private:
	CameraData m_data;
	std::unique_ptr<ICameraMode> m_currentMode;

public:

	void SetMode(std::unique_ptr<ICameraMode> newMode)
	{
		m_currentMode = std::move(newMode);
	}

	void Update(float deltaTime)
	{
		if (m_currentMode)
		{
			m_currentMode->Update(m_data, deltaTime);
		}
	}

	DirectX::SimpleMath::Matrix GetView() const { return m_data.viewMatrix; }

	DirectX::SimpleMath::Vector3 GetPosition() const { return m_data.position; }
};
