#pragma once
#include "ICameraMode.h"
#include <algorithm>
#include <SimpleMath.h>
#include <DirectXMath.h>
#include <Components/SkinnedModelComponent.h>

namespace HEIN
{

	class ThirdPersonMode : public ICameraMode
	{
	private:

		const DirectX::SimpleMath::Vector3* m_playerTarget;

		SkinnedModelComponent* m_fpsModel;
		SkinnedModelComponent* m_tpsModel;

		float m_pitch;
		float m_yaw;
		float m_boomLength;
		float m_mouseSensitivity;
		float m_targetHeight;

	public:

		ThirdPersonMode(DirectX::SimpleMath::Vector3* playerTarget, SkinnedModelComponent* fpsModel, SkinnedModelComponent* tpsModel)
			: m_playerTarget(playerTarget)
			, m_fpsModel(fpsModel)
			, m_tpsModel(tpsModel)
			, m_pitch(-0.35f)
			, m_yaw(0.0f)
			, m_boomLength(23.0f)
			, m_mouseSensitivity(0.005f)
			, m_targetHeight(15.0f)
		{
		}

		void OnEnter(CameraData& /*data*/) override
		{
			if (m_fpsModel != nullptr) m_fpsModel->SetVisible(false);
			if (m_tpsModel != nullptr) m_tpsModel->SetVisible(true);
		}

		void ProcessInput(const CameraInputState& input) override
		{
			m_yaw += -input.mouseX * m_mouseSensitivity;
			m_pitch += -input.mouseY * m_mouseSensitivity;

			constexpr float maxPitchDown = (DirectX::XMConvertToRadians(5.0f));  // look down
			constexpr float maxPitchUp = -(DirectX::XMConvertToRadians(45.0f));  // look up

			// clamp the pitch 
			m_pitch = std::clamp(m_pitch, maxPitchUp, maxPitchDown);


		}

		void Update(CameraData& outData, float /*deltaTime*/, ICameraController& /*controller*/) override
		{
			DirectX::SimpleMath::Vector3 focalPoint = *m_playerTarget;
			focalPoint.y += m_targetHeight;

			DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
			DirectX::SimpleMath::Vector3 offset = rotation.Backward() * m_boomLength;
			DirectX::SimpleMath::Vector3 shoulderOffset = rotation.Right() * 0.5f;

			outData.position = focalPoint + offset + shoulderOffset;
			outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(
				outData.position,
				focalPoint,
				DirectX::SimpleMath::Vector3::Up
			);

			outData.fov = DirectX::XMConvertToRadians(50.0f);
		}

		bool RequiresRelativeMouse() const override { return true; }
		bool LocksPlayerRotation() const override { return true; }

	};
}