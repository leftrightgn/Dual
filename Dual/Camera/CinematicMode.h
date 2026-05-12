#pragma once
#include "ICameraMode.h"

namespace HEIN
{
	class CinematicMode : public ICameraMode
	{
	private:

		static constexpr float DEFAULT_RADIUS = 8.0f;
		static constexpr float DEFAULT_CURRENTANGLE = 0.0f;
		static constexpr float DEFAULT_ORBITSPEED = 0.5f;
		static constexpr float DEFAULT_CAMERAHEIGHT = 1.5f;
	private:

		DirectX::SimpleMath::Vector3 m_centerofStage;
		float m_radius;
		float m_currentAngle;
		float m_orbitSpeed;
		float m_cameraHeight;

	public:

		CinematicMode(
			  DirectX::SimpleMath::Vector3 centerPoint,
			  float radius = DEFAULT_RADIUS,
			  float currentAngle = DEFAULT_CURRENTANGLE,
			  float orbitSpeed = DEFAULT_ORBITSPEED,
			  float cameraHeight = DEFAULT_CAMERAHEIGHT
		);
	
		// Cinematic mode ignores player input
		void ProcessInput(const CameraInputState& /*input*/) override { return; }

		void Update(CameraData& outData, float deltaTime, ICameraController& controller) override;
		
	};

}