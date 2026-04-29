#pragma once
#include "ICameraMode.h"
#include <algorithm>
#include <DirectXMath.h>
#include <Components/SkinnedModelComponent.h>


using namespace HEIN;

class FirstPersonMode : public ICameraMode
{
private:

	const DirectX::SimpleMath::Vector3* m_playerHeadPosition;

	SkinnedModelComponent* m_fpsModel;
	SkinnedModelComponent* m_tpsModel;

	float m_pitch;
	float m_yaw;
	float m_mouseSensitivity;
	float m_targetHeight;
	float m_boomlenght;

public:
    
	FirstPersonMode(const DirectX::SimpleMath::Vector3* headPos, SkinnedModelComponent* fpsModel, SkinnedModelComponent* tpsModel)
		: m_playerHeadPosition(headPos)
		, m_fpsModel(fpsModel)
		, m_tpsModel(tpsModel)
		, m_pitch(0.0f)
		, m_yaw(0.0f)
		, m_mouseSensitivity(0.005f)
		, m_targetHeight(16.0f)
		, m_boomlenght(5.0f)
	{

	}

	void OnEnter(CameraData& data) override
	{
		if (m_fpsModel != nullptr) m_fpsModel->SetVisible(true);
		if (m_tpsModel != nullptr) m_tpsModel->SetVisible(false);
	}

	void ProcessInput(const CameraInputState& input) override
	{
		m_yaw += -input.mouseX * m_mouseSensitivity;
		m_pitch += -input.mouseY * m_mouseSensitivity;

		
		constexpr float pitchLimit = (DirectX::XMConvertToRadians(85.0f)); 
		m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

		constexpr float yawLimit = (DirectX::XMConvertToRadians(60.0f));
		m_yaw = std::clamp(m_yaw, -yawLimit, yawLimit);
	}

	void Update(CameraData& outData, float deltaTime, ICameraController& controller) override
	{
		outData.position = *m_playerHeadPosition;
		outData.position.y += m_targetHeight;
		outData.position.z -= m_boomlenght;
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
		outData.fov = DirectX::XM_PI / 2.5f;
	}

	bool RequiresRelativeMouse() const override { return true; }
};

