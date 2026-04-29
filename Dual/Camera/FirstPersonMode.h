#pragma once
#include "ICameraMode.h"
#include <algorithm>
#include <DirectXMath.h>
#include <Components/SkinnedModelComponent.h>


namespace HEIN
{

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

		DirectX::SimpleMath::Vector3 m_lockedPosition;

	public:

		FirstPersonMode(const DirectX::SimpleMath::Vector3* headPos, SkinnedModelComponent* fpsModel, SkinnedModelComponent* tpsModel)
			: m_playerHeadPosition(headPos)
			, m_fpsModel(fpsModel)
			, m_tpsModel(tpsModel)
			, m_pitch(0.0f)
			, m_yaw(0.0f)
			, m_mouseSensitivity(0.005f)
			, m_targetHeight(15.0f)
			, m_boomlenght(4.5f)
		{

		}

		void OnEnter(CameraData& /*data*/) override
		{
			if (m_fpsModel != nullptr) m_fpsModel->SetVisible(true);
			if (m_tpsModel != nullptr) m_tpsModel->SetVisible(false);


		}

		void ProcessInput(const CameraInputState& input) override
		{
			m_yaw += -input.mouseX * m_mouseSensitivity;
			m_pitch += -input.mouseY * m_mouseSensitivity;


			constexpr float pitchLimit = (DirectX::XMConvertToRadians(80.0f));
			m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

			/*constexpr float yawLimit = (DirectX::XMConvertToRadians(60.0f));
			m_yaw = std::clamp(m_yaw, -yawLimit, yawLimit);*/
		}

		void Update(CameraData& outData, float /*deltaTime*/, ICameraController& /*controller*/) override
		{
			outData.position = *m_playerHeadPosition;
			outData.position.y += m_targetHeight;



			DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);


			DirectX::SimpleMath::Matrix yawOnly = DirectX::SimpleMath::Matrix::CreateRotationY(m_yaw);
			DirectX::SimpleMath::Vector3 flatForward = yawOnly.Forward();
			DirectX::SimpleMath::Vector3 right = yawOnly.Right();

			flatForward.Normalize();

			outData.position += flatForward * m_boomlenght;
			outData.position += right * -0.3f;

			DirectX::SimpleMath::Vector3 target = outData.position + rotation.Forward();
			DirectX::SimpleMath::Vector3 up = DirectX::SimpleMath::Vector3::Up;

			outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(
				outData.position,
				target,
				up
			);

			outData.fov = DirectX::XMConvertToRadians(90.0f);
		}

		bool RequiresRelativeMouse() const override { return true; }
		bool LocksPlayerRotation() const override { return true; }
	};
}
