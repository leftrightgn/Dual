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

		static constexpr float PITCH = -0.35f;
		static constexpr float YAW = 0.0f;
		static constexpr float BOOM_LENGTH = 23.0f;
		static constexpr float MOUSE_SENSITIVITY = 0.005f;
		static constexpr float TARGET_HEIGHT = 15.0f;

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

		ThirdPersonMode(
			  DirectX::SimpleMath::Vector3* playerTarget,
			  SkinnedModelComponent* fpsModel,
			  SkinnedModelComponent* tpsModel
		);

		void OnEnter(CameraData& data) override;

		void ProcessInput(const CameraInputState& input) override;
	
		void Update(CameraData& outData, float deltaTime, ICameraController& controller) override;

		bool RequiresRelativeMouse() const override { return true; }
		bool LocksPlayerRotation() const override { return true; }

	};
}